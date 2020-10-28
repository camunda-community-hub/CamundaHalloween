#!/usr/bin/python

# Most of this code was put together by John over at nyccnc.com
# He has made his code available so I am adapting it for my use.
# Most of this is the same except I am using a Raspberry Pi Cam,
# and using an arduino to control the servo and LED's.
# LED's are primarily used to indicate what the device is doing.
# I decided to remove the lcd display and it's code.




#generic imports
import time
import os
import RPi.GPIO as GPIO     
import picamera                           #For Raspberry PICAM board 
from time import sleep, strftime
from subprocess import *
from subprocess import call

#imports for email
import imaplib
import smtplib
import email
from email.mime.text import MIMEText
from email.parser import HeaderParser
from email.MIMEMultipart import MIMEMultipart
from email.MIMEBase import MIMEBase
from email.Utils import COMMASPACE, formatdate
from email import Encoders

#############
#Picam Stuff
#############

camera = picamera.PiCamera()

#######################
#Arduino Control Setup - sets all the used GPIOs to outputs and sets them low.
#######################

servo = 23 #Arduino Server Motor Control GPIO pin (connected to digital pin 7 input on the Arduino)
PLED = 24  #Arduino PLED Picture LED Indicator (connected to digital pin 8 input on the Arduino)
MLED = 22  #Arduino MLED Mail LED Indicator (connected to digital pin 6 input on the Arduino)
GPIO.setwarnings (False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(servo, GPIO.OUT)
GPIO.output(servo, False)
GPIO.setup(PLED, GPIO.OUT)
GPIO.output(PLED, False)
GPIO.setup(MLED, GPIO.OUT)
GPIO.output(MLED, False)

##########################
#GMAIL ACCOUNT LOGIN INFO - Change this to the gmail account you setup for your dog
##########################

USERNAME = "email.address@gmail.com"     #your dogs gamil account address  #!/usr/bin/env python
PASSWORD = "Password"                    #the password for that account     

##############################
# Interval for checking email 
##############################

programpause = 600     # of seconds to pause before checking email again (10 minutes)

##########################################################
#FUNCTION TO SEARCH FOR AND BUILD A LIST OF UNREAD EMAILS
##########################################################

temp_list = []

def check_email():
    
    status, email_ids = imap_server.search(None, '(UNSEEN)')    
    if email_ids == ['']:
        print('No Unread Emails')
	print('I will check again, in 5 minutes.')
        time.sleep(2)
        mail_list = []
    else:
       
        mail_list = get_senders(email_ids)
        print('List of email senders: ', mail_list)         #FYI when calling the get_senders function, the email is marked as 'read'
        print len(mail_list),
        print("new treat emails!")
	
    imap_server.close()
    time.sleep(2)
    
    return mail_list
     
##########################################
#FUNCTION TO SCRAPE SENDER'S EMAIL ADDRESS   
##########################################

def get_senders(email_ids):
    senders_list = []                                   #creates senders_list list 
    for e_id in email_ids[0].split():                   #Loops IDs of a new emails created from email_ids = imap_server.search(None, '(UNSEEN)')
     resp, data = imap_server.fetch(e_id, '(RFC822)')   #FETCH command retrieves data associated with a message in the mailbox.  
     perf = HeaderParser().parsestr(data[0][1])         #parsing the headers of message
     senders_list.append(perf['From'])                  #Looks through the data parsed in "perf", extracts the "From" field 
    return senders_list

##########################################
#FUNCTION TO SEND THANK-YOU EMAILS
##########################################

def send_thankyou(mail_list):
      print("Sending Emails")
      GPIO.output(MLED, True)
      for item in mail_list:
        files = []
        files.append("pic.jpg")
        text = 'Thank you very much for the treat, I really appreciate it... Delicious!!!'
        assert type(files)==list
        msg = MIMEMultipart()
        msg['Subject'] = 'So Nice of you to think of me!'
        msg['From'] = USERNAME
        msg['To'] = item
        msg.attach ( MIMEText(text) )  

        for file in files:
            part = MIMEBase('application', "octet-stream")
            part.set_payload( open(file,"rb").read() )
            Encoders.encode_base64(part)
            part.add_header('Content-Disposition', 'attachment; filename="%s"'
            % os.path.basename(file))
            msg.attach(part)


        server = smtplib.SMTP('smtp.gmail.com:587')
        server.ehlo_or_helo_if_needed()
        server.starttls()
        server.ehlo_or_helo_if_needed()
        server.login(USERNAME,PASSWORD)
        server.sendmail(USERNAME, item, msg.as_string() )
        server.quit()
        GPIO.output(MLED, False)
        print('Email sent to ' + item + '!')


##############################
#FUNCTION TO DISPENSE TREATS
##############################

def dispensetreats():
    print("Dispensing Treat")
    GPIO.output(servo, True)   #Dispense a few treats 
    sleep(.5) 
    GPIO.output(servo, False)

  
##########################
#FUNCTION TO TAKE PICTURE
##########################

def takepicture():
    time.sleep(3)
    print("Taking Picture")
    GPIO.output(PLED, True)
    camera.capture('pic.jpg')
    GPIO.output(PLED, False)    
    time.sleep (1)
    camera.close
    print("Picture Taken")
    time.sleep(1)

######################
# CHECKING FOR EMAILS
######################

while True:
    print "Checking for emails"
    GPIO.output(MLED, True)
    time.sleep(1)
    imap_server = imaplib.IMAP4_SSL("imap.gmail.com",993)
    imap_server.login(USERNAME, PASSWORD)
    imap_server.select('INBOX')
    mail_list = check_email()
    temp_list = temp_list + mail_list
    GPIO.output(MLED, False)
    if mail_list :
        dispensetreats()            # Dispensing the Treat 
        time.sleep(1)
        takepicture() 
        send_thankyou(mail_list)  
	print ("I will check again, in 5 minutes.")          
    time.sleep(programpause)
