/*global define */

// API_URL: 'http://granslive-web.mybluemix.net/api'
// API_URL: 'http://localhost:3000/api'

define(['angular'], function (angular) {
	'use strict';
  
	return angular.module('app.config', [])
		.constant('CONFIG', {
			VERSION: '0.1',
			ENVIRONMENT: 'OFFICE',
			API_URL: '//localhost:3000/api',
			IOT_CONFIG:{
				"org": "o6oosq",
			    "id": "a-o6oosq-gwvhfgityg",
			    "authkey": "a-o6oosq-gwvhfgityg",
			    "authtoken": "xwottObtqR@WHSe+q-",
			    "type": "shared",
			    "gatewayType": "GransLiveGateway"
			}
		});
    
});