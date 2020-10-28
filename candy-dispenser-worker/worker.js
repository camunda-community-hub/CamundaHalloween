var mqtt = require('mqtt')
var mqttClient = mqtt.connect('mqtt://YOURBROKER')
const { Client, logger } = require('camunda-external-task-client-js');
const open = require('open');

// configuration for the Client:
//  - 'baseUrl': url to the Process Engine
//  - 'logger': utility to automatically log important events
//  - 'asyncResponseTimeout': long polling timeout (then a new request will be issued)
const config = { baseUrl: 'http://localhost:8080/engine-rest', use: logger, asyncResponseTimeout: 10000 };

// create a Client instance with custom configuration
const client = new Client(config);

client.subscribe('costumes', async function ({ task, taskService }) {

  candy = task.variables.get('candyPieces');
  console.log('Dispensing ' + candy + ' pieces of candy!');
  var cString = '{candy=' + candy + '}';
  mqttClient.publish('candy', cString);
  // Complete the task
  await taskService.complete(task);
});