const char dashboard_index_html[] = R"=====(
<!DOCTYPE html>
<head>
    <title>OpenHaldex</title>
    <link rel="icon" type="image/x-icon" href="/favicon.ico">
    <meta name='viewport' content='width=device-width,initial-scale=0.5,maximum-scale=1,user-scalable=yes' charset="utf-8">
</head>

<script>
const S_wifi = "Wi-Fi";
const S_update_title = "Update";

const HTML_strings = {
  "S_wifi":S_wifi,
  "S_update_title":S_update_title,
};

const S_target = "Target";
const S_actual = "Actual";
</script>

<style>
    body {
        display: flex;
        flex-direction: column;
        justify-content: center;
        align-items: center;
        background-image: url('/media/background.jpeg');
        font-family: sans-serif;
        color: white;
    }

    form {
        display: flex;
        justify-content: center;
        text-align: center;
        background: black;
        border-radius: 20px;
        padding: 20px;
        width: 80%;
        margin: 10px;
    }

    table {
        width: 100%;
        table-layout: fixed;
        font-size: 50px;
    }

    td {
        width: 50%;
        padding: 10px 10px 10px 10px;
    }

    th {
        text-align: center;
    }

    label {
        font-size: 20px;
        display: block;
    }

    input.btn {
        white-space: normal;
        text-overflow: ellipsis;
        overflow: hidden;
        
        font-family: monospace;
        background: #b5e61d;
        color: black;
        cursor: pointer;
        width: 100%;
        border-radius: 10px;
        border: none;
        font-size: 35px;
        font-weight: bold;
        
        padding: 0 0 0 0;
        margin: 0 0 0 0;
    }
    
    #btn_stock
    {
        box-shadow: inset 0 0 10px 2px red, 0 0 10px 5px red;
    }
    
    #btn_fwd
    {
        box-shadow: inset 0 0 10px 2px green, 0 0 10px 5px green;
    }
    
    #btn_5050
    {
        box-shadow: inset 0 0 10px 2px blue, 0 0 10px 5px blue;
    }
    
    #btn_7525
    {
        box-shadow: inset 0 0 10px 2px white, 0 0 10px 5px white;
    }

    #prgbar
    {
        background-color: gray;
        border-radius: 10px;
    }

    #bar_target,
    #bar_actual
    {
        background-color: #b5e61d;
        border-radius: 10px;
        width: 0%;
        height: 20px;
    }

    #prg_target,
    #prg_actual
    {
        font-size: 20px;
    }
    
    #mode_div,
    #speed_div
    {
      color: #b5e61d;
    }
</style>

<script src='/jquery.min.js'></script>

<body onload='load_strings(), resize(), get_ip()'>
    <img src="/media/logo.png" height='400'>
    
    <br>
    
    <form>
        <table>
            <tr>
                <td>
                    <label>Mode</label>
                    <div id='mode_div'>?</div>
                </td>
            </tr>
            <tr>
              <td>
                <hr>
              </td>
            </tr>
            <tr>
                <td>
                    <label id='target_label'></label>
                    <div id='prgbar'>
                        <div id='bar_target'></div>
                    </div>
                    <div id='prg_target'>0%</div>
                </td>
            </tr>
            <tr>
              <td>
                <hr>
              </td>
            </tr>
            <tr>
                <td>
                    <label id='actual_label'></label>
                    <div id='prgbar'>
                        <div id='bar_actual'></div>
                    </div>
                    <div id='prg_actual'>0%</div>
                </td>
            </tr>
            <tr>
              <td>
                <hr>
              </td>
            </tr>
            <tr>
                <td>
                    <label>Speed</label>
                    <div id='speed_div'>0km/h</div>
                </td>
            </tr>
        </table>
    </form>
    
    <br>
    
    <form>
        <table>
            <tr>
                <td>
                    <input type='button' id='btn_stock' onclick='set_mode(0)' class='btn' value='STOCK'>
                </td>
                <td>
                    <input type='button' id='btn_fwd' onclick='set_mode(1)' class='btn' value='FWD'>
                </td>
            </tr>
            <tr>
                <td>
                    <input type='button' id='btn_5050' onclick='set_mode(2)' class='btn' value='5050'>
                </td>
                <td>
                    <input type='button' id='btn_7525' onclick='set_mode(3)' class='btn' value='7525'>
                </td>
            </tr>
        </table>
    </form>
    
    <br>
    
    <form>
        <table>
            <tr>
                <th colspan=2 id='ip_th'></th>
            </tr>
        </table>
    </form>

    <script>
        var source = new EventSource('/sse');
        source.addEventListener('data', parse_sse_data, false);
        
        window.addEventListener('resize', resize, true);
        
        function load_strings() {
          for (const str in HTML_strings) {
            var elms = document.querySelectorAll("#" + str);
            for(var i = 0; i < elms.length; i++) {
              elms[i].insertAdjacentHTML("beforeend", HTML_strings[str]);
            }
          }
          
          document.getElementById('target_label').innerHTML = S_target;
          document.getElementById('actual_label').innerHTML = S_actual;
        }
        
        function resize()
        {
          const collection = document.getElementsByClassName('btn');
          for (let i = 0; i < collection.length; i++)
          {
            collection[i].style.height = window.getComputedStyle(collection[i]).getPropertyValue('width');
          }
        }
        
        function get_ip()
        {
            var xhr = new XMLHttpRequest();

            xhr.onreadystatechange = function ()
            {
                if (xhr.readyState == 4 && xhr.status == 200)
                {
                  document.getElementById('ip_th').innerHTML = xhr.responseText;
                }
            };

            xhr.open('POST', '/ip', true);
            xhr.send();
        }
    
        function parse_sse_data(event)
        {
          var event_data = event.data;
          var message_fields_array = event_data.split(",");
          var mode = parseInt(message_fields_array[0], 16);
          var target = parseInt(message_fields_array[1], 16);
          var actual = parseInt(message_fields_array[2], 16);
          var speed = parseInt(message_fields_array[3], 16);
          //console.log("mode: " + mode + "\ntarget: " + target + "\nactual: " + actual + "\nspeed: " + speed);
          
          document.getElementById('mode_div').innerHTML = get_openhaldex_mode_string(mode);
          
          var target_percent = Math.round(target * 100 / 255) + '%';
          document.getElementById('prg_target').innerHTML = target_percent;
          document.getElementById('bar_target').style.width = target_percent;
          
          var actual_percent = Math.round(actual * 100 / 255) + '%';
          document.getElementById('prg_actual').innerHTML = actual_percent;
          document.getElementById('bar_actual').style.width = actual_percent;
          
          document.getElementById('speed_div').innerHTML = speed + "km/h";
          
          var color_stock = "#b5e61d";
          var color_fwd = "#b5e61d";
          var color_5050 = "#b5e61d";
          var color_7525 = "#b5e61d";
          
          var current_mode_string = document.getElementById('mode_div').innerHTML;
          switch (current_mode_string)
          {
            case "STOCK":
              color_stock = "#50660a";
              break;
            case "FWD":
              color_fwd = "#50660a";
              break;
            case "5050":
              color_5050 = "#50660a";
              break;
            case "7525":
              color_7525 = "#50660a";
              break;
          }
          
          document.getElementById('btn_stock').style.background = color_stock;
          document.getElementById('btn_fwd').style.background = color_fwd;
          document.getElementById('btn_5050').style.background = color_5050;
          document.getElementById('btn_7525').style.background = color_7525;
        }
        
        function get_openhaldex_mode_string(mode)
        {
          switch (mode)
          {
            case 0:
              return "STOCK";
            case 1:
              return "FWD";
            case 2:
              return "5050";
            case 3:
              return "7525";
            case 4:
              return "CUSTOM";
            default:
              break;
          }
          return "?";
        }
        
        function set_mode(mode)
        {
          const post_request = '' + mode;
          
          const xhr = new XMLHttpRequest();
          xhr.open("POST", "/set_mode", false);
          xhr.send(post_request);
        }
    </script>
</body>
)=====";
