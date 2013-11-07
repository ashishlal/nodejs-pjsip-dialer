var addon = require('./build/Release/dialer');

/**
* Init function.
*
*/

module.exports = function init () {
    console.log("calling init....");
	addon.init();
};


