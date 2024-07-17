// Get current sensor readings when the page loads  
window.addEventListener('load', getReadings);

// Create Load Gauge
var gaugeLoad = new RadialGauge({
  renderTo: 'load-gauge',
  minValue: 0,
  maxValue: 150,
  animation: true,
  units: "(Kgs)",
  majorTicks: [
      "0",
      "20",
      "40",
      "60",
      "80",
      "100",
      "120",
      "140"
  ],
}).draw();

// var gaugeMax = new RadialGauge({
//   renderTo: 'max-gauge',
//   minValue: 0,
//   maxValue: 150,
//   animation: true,
//   units: "(Kgs)",
//   majorTicks: [
//       "0",
//       "20",
//       "40",
//       "60",
//       "80",
//       "100",
//       "120",
//       "140"
//   ],
// }).draw();


// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      var load = myObj.load;
      // var maxLoad = myObj.max;
      gaugeLoad.value = load;
      // gaugeMax.value = maxLoad;
    }
  }; 
  xhr.open("GET", "/readings", true);
  xhr.send();
}

if (!!window.EventSource) {
  var source = new EventSource('/events');
  
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);
  
  source.addEventListener('new_readings', function(e) {
    var myObj = JSON.parse(e.data);
    var load = myObj.load;
    // var maxLoad = myObj.max;
    gaugeLoad.value = load;
    // gaugeMax.value = maxLoad;
  }, false);
}
  
// // Create Humidity Gauge
// var gaugeHum = new RadialGauge({
//   renderTo: 'gauge-humidity',
//   width: 300,
//   height: 300,
//   units: "Humidity (%)",

//   colorValueBoxRect: "#049faa",
//   colorValueBoxRectEnd: "#049faa",
//   colorValueBoxBackground: "#f1fbfc",
//   valueInt: 2,
//   majorTicks: [
//       "0",
//       "20",
//       "40",
//       "60",
//       "80",
//       "100"

//   ],
//   minorTicks: 4,
//   strokeTicks: true,
//   highlights: [
//       {
//           "from": 80,
//           "to": 100,
//           "color": "#03C0C1"
//       }
//   ],
//   colorPlate: "#fff",
//   borderShadowWidth: 0,
//   borders: false,
//   needleType: "line",
//   colorNeedle: "#007F80",
//   colorNeedleEnd: "#007F80",
//   needleWidth: 2,
//   needleCircleSize: 3,
//   colorNeedleCircleOuter: "#007F80",
//   needleCircleOuter: true,
//   needleCircleInner: false,
//   animationDuration: 1500,
//   animationRule: "linear"
// }).draw();