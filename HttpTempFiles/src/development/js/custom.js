$(document).on("pageinit",function() {
    getSensorInfo(5, 1);
    getSensorInfo(5, 2);
    getSettings();
    
    setInterval(function(){ 
        $("#sensor"+1+"List li").remove();
        $("#sensor"+2+"List li").remove();
        getSensorInfo(5, 1);
        getSensorInfo(5, 2);
        getSettings();
    }, 5000);
});

function getSensorInfo(count, sensor){
    $.ajax({
        type: "POST",
        url: "http://beaglebone-eth:8888/getTemp",
        data : {"count" : count, "sensor": sensor},
        dataType : "json",
        success: function(data) {
            
            $("#sensor"+sensor+"FirstTemp").html(data[0].temperature+"&deg;");
            
            $.each(data, function(i){
                 if(i == 0) return;
                 var date = new Date(data[i].timestamp*1000);
                 $("#sensor"+sensor+"List").append("<li><span>"+date.getHours()+":"+date.getMinutes()+"</span><strong>"+data[i].temperature+"&deg;</strong></li>");
            });
            
        }
    }); 
}

function getSettings(){
    $.ajax({
        type: "POST",
        url: "http://beaglebone-eth:8888/getSettings",
        data : {"type" : "sensors"},
        dataType : "json",
        success: function(data) {   
            $("#circleSensor1").attr('class', (data.sensor1) ? "icon-circle sunny" : "icon-circle cold");
            $("#circleSensor2").attr('class', (data.sensor2) ? " icon-circle sunny" : "icon-circle cold");
            
            $("#statusSensor1").html((data.sensor1)  ? "STATUS: ON" : "STATUS: OFF");
            $("#statusSensor2").html((data.sensor2)  ? "STATUS: ON" : "STATUS: OFF");
        }
    }); 
}