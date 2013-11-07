/**
* Module requirements.
*/
var express = require('express')
	, register = require('./register')
	, call = require('./call')

/**
* Create app.
*/
var app = express.createServer();
/**
* Configuration
*/
app.set('view engine', 'ejs');
app.set('views', __dirname + '/views');
app.set('view options', { layout: false });
/**
* Routes
*/
app.get('/', function (req, res) {
	res.render('index');
});

app.get('/register', function (req, res, next) {
  register(req.query.domain, req.query.user, req.query.password, res);
});
app.get('/call', function (req, res) {
  call(req.query.callee, res);
});
/**
* Listen
*/
app.listen(3000);
