var addon = require('./build/Release/dialer');

/**
* Init function.
*
*/

module.exports = function poll () {
	addon.poll();
};


