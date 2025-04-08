const char update_index_html[] = R"=====(
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

const S_wifi_ssid_placeholder = "Wi-Fi network name";
const S_wifi_pass_placeholder = "Wi-Fi network password";

const S_wifi_ssid_constraints = "The Wi-Fi network name must contain between 1-31 characters"
const S_wifi_pass_constraints = "The Wi-Fi network password must contain between 8-63 characters"

const S_select_bin_file = "Select .bin file";
const S_set = "Set";
const S_update_button = "Update";
const S_factory_reset_button = "Factory reset";

const S_connection_closed = "Connection closed!";

const S_select_a_file = "Please select a file.";
const S_select_bin = "Please select a .bin file.";
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
    }

    th {
        text-align: center;
    }

    label {
        font-size: 20px;
        display: block;
    }

    #ssid_text_input,
    #ssid_pass_input,
    #file-input
    {
        white-space: nowrap;
        text-overflow: ellipsis;
        overflow: hidden;
        
        height: 44px;
        border-radius: 10px;
        margin: 10px auto;
        font-size: 20px;
        padding: none;
        border: 1px solid #ddd;
        line-height: 45px;
        text-align: center;
        display: block;
        cursor: pointer;
    }
    
    #ssid_text_input,
    #ssid_pass_input
    {
      width: 100%;
    }

    input.btn {
        white-space: normal;
        text-overflow: ellipsis;
        overflow: hidden;
        
        font-family: monospace;
        background: #b5e61d;
        color: black;
        cursor: pointer;
        height: auto;
        width: 90%;
        border-radius: 10px;
        border: none;
        font-size: 35px;
        margin: 5px auto;
        padding-top: 10px;
        padding-bottom: 10px;
    }

    #prg_update
    {
        font-size: 20px;
    }

    #prgbar {
        background-color: #f1f1f1;
        border-radius: 10px;
    }

    #bar_update
    {
        background-color: #3498db;
        border-radius: 10px;
        width: 0%;
        height: 20px;
    }
</style>

<script src='/jquery.min.js'></script>

<body onload='load_strings()'>
    <img src="/media/logo.png" height='400'>
    
    <br>
    
    <form id='wifi_form'>
        <table>
            <tr>
                <th colspan=2 id='S_wifi'></th>
            </tr>
            <tr>
                <td>
                    <input type='text' name='ssid' id='ssid_text_input'>
                    <input type='password' name='pass' id='ssid_pass_input'>
                </td>
                <td>
                    <input type='button' onclick='set_wifi()' id='wifi_set_button' class='btn'
                        {{HIDE_UPDATE_BUTTON}}>
                </td>
            </tr>
        </table>
    </form>
    
    <br>
    
    <form id='update_form'>
        <table>
            <tr>
                <th colspan=2 id='S_update_title'></th>
            </tr>
            <tr>
                <td>
                    <input type='file' name='update' id='firmware-file' onchange='set_update_filename(this)'
                        style='display:none' accept='.bin'>
                    <label id='file-input' for='firmware-file'></label>
                    <div id='prg_update'></div>
                    <div id='prgbar'>
                        <div id='bar_update'></div>
                    </div>
                </td>
                <td>
                    <input type='button' onclick='upload_update()' id='update-button' class='btn'
                        {{HIDE_UPDATE_BUTTON}}>
                </td>
            </tr>
        </table>
    </form>
    
    <br>
    
    <form>
        <table>
            <tr>
                <td>
                    <input type='button' onclick='do_factory_reset()' id='factory_reset_button' class='btn'
                        {{HIDE_UPDATE_BUTTON}}>
                </td>
            </tr>
        </table>
    </form>

    <script>
        function load_strings() {
          for (const str in HTML_strings) {
            var elms = document.querySelectorAll("#" + str);
            for(var i = 0; i < elms.length; i++) {
              elms[i].insertAdjacentHTML("beforeend", HTML_strings[str]);
            }
          }
          
          document.getElementById('wifi_set_button').value = S_set;
          document.getElementById('file-input').innerHTML = S_select_bin_file;
          document.getElementById('update-button').value = S_update_button;
          document.getElementById('factory_reset_button').value = S_factory_reset_button;
          
          document.getElementById('ssid_text_input').placeholder = S_wifi_ssid_placeholder;
          document.getElementById('ssid_pass_input').placeholder = S_wifi_pass_placeholder;
        }

        function set_update_filename(obj) {
            var fileName = obj.value.match(/[^\\/]*$/)[0];
            fileName = fileName.slice(0, -4);
            document.getElementById('file-input').innerHTML = fileName;
        }

        // S Function called when pressing the button for setting Wi-Fi parameters
        function set_wifi() {
          var ssid_text_input = document.getElementById('ssid_text_input');
          var ssid_pass_input = document.getElementById('ssid_pass_input');
          
          if (ssid_text_input.value == null || ssid_text_input.value == "" || ssid_text_input.length == 0 || ssid_text_input.length > 31)
          {
            alert(S_wifi_ssid_constraints);
            return;
          }
          
          if (ssid_pass_input.value == null || ssid_pass_input.value == "" || ssid_pass_input.length < 8 || ssid_pass_input.length > 63)
          {
            alert(S_wifi_pass_constraints);
            return;
          }
          
          var xhttp = new XMLHttpRequest();
          xhttp.open('POST', '/set_wifi');
          
          var query_string = "ssid=" + encodeURIComponent(ssid_text_input.value) + "&pass=" + encodeURIComponent(ssid_pass_input.value);
          xhttp.send(query_string);
          
          xhttp.onreadystatechange = function () {
              if (xhttp.readyState == 4) {
                  if (xhttp.status == 0) {
                      alert(S_connection_closed);
                  } else {
                      alert(xhttp.responseText);
                  }
              }
          };
        }
        // E Function called when pressing the button for setting Wi-Fi parameters

        // S Function called when pressing the button for update
        function upload_update() {
            var fileInput = document.getElementById('firmware-file');
            var file = fileInput.files[0];

            if (!file) {
                alert(S_select_a_file);
                return;
            }

            var fileName = file.name;
            var fileExtension = fileName.substring(fileName.lastIndexOf('.') + 1).toLowerCase();

            if (fileExtension !== 'bin') {
                alert(S_invalid_format + ' ' + S_select_bin);
                return;
            }

            var data = new FormData(document.getElementById('update_form'));
            if (data.get('update') === '') {
                alert(S_select_a_file);
                return;
            }

            $('#wifi_set_button').hide();
            $('#factory_reset_button').hide();
            $('#update-button').hide();

            var xhttp = new XMLHttpRequest();
            xhttp.upload.addEventListener('progress', function (evt) {
                if (evt.lengthComputable) {
                    var per = evt.loaded / evt.total;
                    var per_scaled = per * 100;
                    per_scaled = Math.min(per_scaled, 99);
                    $('#prg_update').html(Math.round(per_scaled) + '%');
                    $('#bar_update').css('width', Math.round(per_scaled) + '%');
                }
            }, false);

            xhttp.onreadystatechange = function () {
                if (xhttp.readyState == 4) {
                    $('#prg_update').html('100%');
                    $('#bar_update').css('width', '100%');
                    
                    if (xhttp.status == 0) {
                        alert(S_connection_closed);
                    } else {
                        alert(xhttp.responseText);

                        var end_update_xhttp = new XMLHttpRequest();
                        end_update_xhttp.open('GET', '/end_update', true);
                        end_update_xhttp.send();
                    }
                }
            };

            xhttp.open('POST', '/update', true);
            xhttp.send(file);
        }
        // E Function called when pressing the button for update
        
        function do_factory_reset()
        {
          if (confirm("Are you sure?"))
          {
            var xhr = new XMLHttpRequest();
            xhr.open('GET', '/format', true);
            xhr.send();
          }
        }
    </script>
</body>
)=====";
