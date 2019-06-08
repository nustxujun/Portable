// This file is required by the index.html file and will
// be executed in the renderer process for that window.
// All of the Node.js APIs are available in this process.



var slider = window['vue-slider-component'];
var context = {
    message: "等待连接...",
    stages:{},
    properties: {}
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
        context.properties ={};
        context.stages ={};

    }
    else if (data.type == "set") {
        context.properties[data.key] = {
            value: +data.value, 
            min: +data.min, 
            max: +data.max,
            interval : +data.interval,
            change(value){

                network.send({type:"set", key:data.key, value:value})
            }
        }
    }
    else if (data.type == "stage")
    {
   
        context.stages[data.key] = {
            cost: +data.cost, 
        }


    }

    if (data.msg) {
        context.message = data.msg;
    }
})