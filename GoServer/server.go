package main

import (
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"strings"
	"time"

	camundaclientgo "github.com/citilinkru/camunda-client-go"
)

const prefix = "http://localhost:9090"

// Error represents a handler error. It provides methods for a HTTP status
// code and embeds the built-in error interface.
type Error interface {
	error
	Status() int
}

// StatusError represents an error with an associated HTTP status code.
type StatusError struct {
	Code int
	Err  error
}

// Allows StatusError to satisfy the error interface.
func (se StatusError) Error() string {
	return se.Err.Error()
}

// Status Returns our HTTP status code.
func (se StatusError) Status() int {
	return se.Code
}

// sendPic sends the picture to the Camunda Process.
func sendPic(s string) {
	client := camundaclientgo.NewClient(camundaclientgo.ClientOptions{
		EndpointUrl: "http://localhost:8080/engine-rest",
		ApiUser:     "demo",
		ApiPassword: "demo",
		Timeout:     time.Second * 10,
	})
	newPath := prefix + strings.Trim(s, ".")
	processKey := "costumes"
	variables := map[string]camundaclientgo.Variable{
		"newCostume":  {Value: newPath, Type: "string"},
		"isCostume":   {Value: true, Type: "boolean"},
		"candyPieces": {Value: 0, Type: "long"},
	}
	_, err := client.ProcessDefinition.StartInstance(
		camundaclientgo.QueryProcessDefinitionBy{Key: &processKey},
		camundaclientgo.ReqStartInstance{Variables: &variables},
	)
	if err != nil {
		log.Printf("Error starting process: %s\n", err)
		return
	}
	//log.Printf("Result: %#+v\n", result)
}

func photo(w http.ResponseWriter, r *http.Request) {
	if r.Method == "GET" {
		log.Println("GET Method Not Supported")
		http.Error(w, "GET Method not supported", 400)
	} else {
		r.ParseMultipartForm(32 << 2) // allocate enough memory for the incoming picture.
		file, handler, err := r.FormFile("uploadfile")
		if err != nil {
			fmt.Println("Format Error!", err)
			switch e := err.(type) {
			case Error:
				// We can retrieve the status here and write out a specific
				// HTTP status code.
				log.Printf("HTTP %d - %s", e.Status(), e)
				http.Error(w, e.Error(), e.Status())
			default:
				// Any error types we don't specifically look out for default
				// to serving a HTTP 500
				http.Error(w, http.StatusText(http.StatusInternalServerError),
					http.StatusInternalServerError)
			}
			return
		}
		defer file.Close()
		log.Println("Incoming file: ", handler.Filename)
		f, err := os.OpenFile("./test"+handler.Filename, os.O_WRONLY|os.O_CREATE, 0666)
		if err != nil {
			log.Println(err)
			http.Error(w, "Could not Write new file", 500 )
			return
		}
		defer f.Close()
		io.Copy(f, file)
		sendPic(f.Name())
		w.WriteHeader(200)
	}
}

func main() {
	fmt.Println("Starting up ... ")
	fs := http.FileServer(http.Dir("/Users/davidgs/github.com/CamundaHalloween/GoServer/test/"))

	http.HandleFunc("/photo", photo)
	http.Handle("/test/", http.StripPrefix("/test", fs)) // set router
	err := http.ListenAndServe(":9090", nil)             // set listen port
	if err != nil {
		log.Fatal("ListenAndServe: ", err)
	}
}
