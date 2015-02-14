/*
 * Communication with Watchapp
 *****************************************/

Pebble.addEventListener("ready",
  function(e) {
    GoogleAPI.init();
    console.log("Hello world! - code: " + GoogleAPI.refresh_token);
  }
);

Pebble.addEventListener('appmessage',
  function(e) {
    console.log('Received message: ' + e.payload);
    GoogleAPI.step = 1;
    GoogleAPI.getToken();
    console.log('Access token: ' + GoogleAPI.access_token);
  }
);

var GoogleAPI = {
  CLIENT_ID: '481485418952-ltb4vsnr5n4mjgosei7p7ium1er3dsqu.apps.googleusercontent.com',
  CLIENT_SECRET: 'Mzs4X1oDp0PD76UT6ggRG2Ol',
  GOOGLE_FIT_API: 'https://www.googleapis.com/fitness/v1/users/me/dataSources/',
  dataStreamId: 'derived:com.google.step_count.delta:407408718192:Pebble:Pebble:1000001',

  refresh_token: '',
  access_token: '',
  step: 0,

  init: function() {
    this.refresh_token = localStorage.getItem('refresh_token');
  },

  getToken: function() {
    var xhr = new XMLHttpRequest();
    var params = {
      'refresh_token': this.refresh_token,
      'client_id': this.CLIENT_ID,
      'client_secret': this.CLIENT_SECRET,
      'grant_type': 'refresh_token'
    }
    var paramstr = '';
    for (key in params) {
      paramstr += key + '=' + encodeURIComponent(params[key]) + '&';
    }
    xhr.onload = function() {
      var json = JSON.parse(this.responseText);
      GoogleAPI.access_token = json.access_token;
      GoogleAPI.sendData();
    }
    xhr.open('POST', 'https://www.googleapis.com/oauth2/v3/token', true);
    xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xhr.send(paramstr);
  },

  sendData: function() {
    var startTime = (Date.now()-10000) + '000000',
        endTime = (Date.now()) + '000000';
    var params = {
      "dataSourceId": this.dataStreamId,
      "minStartTimeNs": startTime,
      "maxEndTimeNs": endTime,
      "point": [
        {
          "startTimeNanos": startTime,
          "endTimeNanos": endTime,
          "dataTypeName": "com.google.step_count.delta",
          "value": [{"intVal": this.step}]
        }
      ]
    }
    var url = this.GOOGLE_FIT_API + this.dataStreamId + '/datasets/' +
      startTime + '-' + endTime;
    var xhr = new XMLHttpRequest();
    xhr.onload = function() {}
    xhr.open('PATCH', url, true);
    xhr.setRequestHeader("Content-type", "application/json");
    xhr.setRequestHeader("Authorization", "Bearer " + this.access_token);
    xhr.send(JSON.stringify(params));
  }
}

/*
 * App Configurations
 *****************************************/

// Show config page
Pebble.addEventListener('showConfiguration', function(e) {
  var configuration = JSON.stringify(localStorage.getItem('conf'))
  Pebble.openURL('https://robotshell.org/pebbly-fit/configuration.html#' + configuration);
});

// Return from configuration
Pebble.addEventListener('webviewclosed', function(e) {
  console.log('Configuration window returned: ' + e.response);
  var configuration = JSON.parse(decodeURIComponent(e.response));
  localStorage.setItem('refresh_token', configuration.refresh_token);
});
