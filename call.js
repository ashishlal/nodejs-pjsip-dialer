var addon = require('./build/Release/dialer');

/**
* Call function.
*
*/

module.exports = function call (callee, res) {
	var resp = addon.call(callee);
	res.send(resp);
};


