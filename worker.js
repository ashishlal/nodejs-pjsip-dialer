//var addon = require('./build/Release/dialer');
function messageHandler(event) {

    // Accessing to the message data sent by the main page
	var messageSent = event.data;
	switch(messageSent.Message) 
	{
		case "init":
			console.log("init");
			init();
			break;
		case "register":
			console.log("register");
			//register(messageSent.D, messageSent.U, messageSent.P);
			break;
		case "poll":
			console.log("poll");
			//poll();
			break;
        default:
			console.log("default case");
			break;
	}
}

// Defining the callback function raised when the
this.addEventListener('message', messageHandler, false);
