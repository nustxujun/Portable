// This file is required by the index.html file and will
// be executed in the renderer process for that window.
// All of the Node.js APIs are available in this process.



var slider = window[ 'vue-slider-component' ];
var context = {message :""};

var app = new Vue({ 
    el: '#app',
    data : context,

    components: {
        'vueSlider': slider,
    }
});

      
var network = require("./Network.js");
network.init(8888);
network.receive(function (data)
{
    context.message = data.message
})