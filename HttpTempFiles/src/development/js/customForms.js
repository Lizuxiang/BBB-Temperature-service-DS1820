$(document).on("pageinit",function() {
    getSensorInfo(5, 1);
    
    $("#settings").submit(function() {

    var post = {};
    post["sensor1Enabled"] = ($("#flip2b").val() == 'on') ? 1: 0;
    post["sensor2Enabled"] = ($("#flip2a").val() == 'on') ? 1: 0;
    post["temperatureAlarmMin"] = $("#range1a").val();
    post["temperatureAlarmMax"] = $("#range1b").val();
    post["notifyPerioidMinutes"] = $("#range1c").val();
    post["NUM_SECONDS"] = $("#range1d").val();
    
    $.ajax({
           type: "POST",
           url: "http://beaglebone-eth:8888/setSettings",
           data : post,
           dataType : "json",
           success: function(data)
           {
               
           }
         });

    return false; // avoid to execute the actual submit of the form.
    });
});

function getSensorInfo(count, sensor){
    $.ajax({
        type: "POST",
        url: "http://beaglebone-eth:8888/getSettings",
        data : {"type" : "all"},
        dataType : "json",
        success: function(data) {
            
            $("#flip2b").val((data.sensor1) ? 'on' : 'off').slider('refresh');
            $("#flip2a").val((data.sensor2) ? 'on' : 'off').slider('refresh');    
            
            $("#range1a").val(data.temperatureAlarmMin);
            $("#range1b").val(data.temperatureAlarmMax).slider('refresh');
            
            $("#range1c").val(data.notifyPerioidMinutes).slider('refresh');
            $("#range1d").val(data.NUM_SECONDS).slider('refresh');
        }
    }); 
}