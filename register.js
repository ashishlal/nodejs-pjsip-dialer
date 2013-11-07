var addon = require('./build/Release/dialer');

/**
* Register function.
*
*/

module.exports = function register (domain, uname, pass, res) {
	var resp = addon.register(domain, uname, pass);
	if(resp == "REGISTERED") {
		res.render('call');
	}
};


