// This file is required by the index.html file and will
// be executed in the renderer process for that window.
// All of the Node.js APIs are available in this process.



var slider = window['vue-slider-component'];
var context = {
    message: "",
    properties: [
        {
            key: "roughness",
            value: 0.22,
            min: 0,
            max: 1,
            interval : 0.001,
            change(data){
                console.log(data)
            }
        }
    ],
};

var app = new Vue({
    el: '#app',
    data: context,
    methods:{
    },
    components: {
        'vueSlider': slider,
    }
});


var network = require("./Network.js");
network.init(8888);
network.receive(function (data) {
    if (data.type == "init") {
        context.properties.length = 0;
    }
    else if (data.type == "set") {
        context.properties.push({
            key: data.key, 
            value: +data.value, 
            min: +data.min, 
            max: +data.max,
            interval : +data.interval,
            change(value){

                network.send({type:"set", key:data.key, value:value})
            }
        })
    }

    if (data.msg) {
        context.message = data.msg;
    }
})