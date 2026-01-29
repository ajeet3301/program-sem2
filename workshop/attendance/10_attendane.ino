#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <vector> 

// ---------------- CONFIGURATION ----------------
const char* ssid = "Class_Attendance";       
const char* password = "password123";       
const char* admin_user = "teacher";         
const char* admin_pass = "admin123";        

// ---------------- STUDENT DATA ----------------
String students[] = {
  "255ICS001: Kartik Agnihotri", "255ICS002: Deepanshu", "255ICS003: Meenakshi",
  "255ICS004: Bhavya Singh Rajawat", "255ICS005: Deepak Kumar Mishra", "255ICS006: Jiya Sharma",
  "255ICS007: Yug", "255ICS008: Shivam", "255ICS009: Tanisha", "255ICS010: Anshuman Gautam",
  "255ICS011: Shashank Gupta", "255ICS012: Devkaran", "255ICS013: Nidhi Singh",
  "255ICS014: Ajeet Kumar", "255ICS015: Krishan Kant Singh", "255ICS016: Shivam Kumar",
  "255ICS017: Manish Kemwall", "255ICS018: Abhishek Garg", "255ICS019: Keshav Balyan",
  "255ICS020: Soham Anand", "255ICS021: Abhi Teotia", "255ICS022: Aman Kumar",
  "255ICS023: Harsh Singh", "255ICS024: Reva Mudgil", "255ICS025: Priyanshu Raj"
};
int totalStudents = 25;

DNSServer dnsServer;
ESP8266WebServer server(80);

// ---------------- MAIN PAGE HTML ----------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Class Attendance</title>
  <style>
    body {
      margin: 0; padding: 0;
      background: #121212 no-repeat center center fixed;
      background-size: cover;
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      min-height: 100vh; display: flex; flex-direction: column;
      justify-content: center; align-items: center; color: white;
    }
    .card {
      width: 85%; max-width: 380px;
      padding: 25px; border-radius: 20px;
      background: rgba(0, 0, 0, 0.75);
      backdrop-filter: blur(12px);
      border: 1px solid rgba(255,255,255,0.1);
      text-align: center; box-shadow: 0 10px 30px rgba(0,0,0,0.5);
    }
    h2 { color: #00d2ff; margin-top: 0; letter-spacing: 1px; }
    select {
      width: 100%; padding: 12px; margin-bottom: 15px;
      background: #1a1a1a; border: 1px solid #444;
      border-radius: 10px; color: #fff; font-size: 15px; outline: none;
    }
    .btn-main {
      width: 100%; padding: 13px; border-radius: 10px;
      border: none; background: linear-gradient(135deg, #00d2ff, #3a7bd5);
      color: white; font-weight: bold; font-size: 16px; cursor: pointer;
      transition: 0.3s;
    }
    .btn-main:active { transform: scale(0.98); }
    
    .live-list { margin-top: 20px; text-align: left; border-top: 1px solid #333; padding-top: 15px; }
    #present-names { max-height: 150px; overflow-y: auto; font-size: 13px; color: #ccc; }
    .p-item { padding: 4px 0; border-bottom: 1px solid rgba(255,255,255,0.05); }

    /* Suspicious Section */
    .btn-sus { 
        width: 100%; margin-top: 15px;
        background: rgba(255, 68, 68, 0.2); border: 1px solid #ff4444; 
        color: #ff4444; padding: 10px; border-radius: 8px; font-size: 14px; 
        cursor: pointer; font-weight: bold;
    }
    #sus-container { display: none; margin-top: 10px; border: 1px solid #ff4444; border-radius: 10px; padding: 10px; background: rgba(50,0,0,0.5); }
    #sus-list { color: #ff8888; font-size: 12px; text-align: left; max-height: 100px; overflow-y: auto; }
    
    /* Small Wallpaper Button */
    .wall-btn {
        background: none; border: none; color: #666; 
        font-size: 11px; margin-top: 10px; cursor: pointer; text-decoration: underline;
    }
    #bgInput { display: none; }

    .dev-container { margin-top: 20px; font-size: 11px; color: #555; border-top: 1px solid #222; padding-top: 10px;}
    .dev-btn { cursor: pointer; text-decoration: none; display: inline-block; padding: 5px; }
    .dev-list { display: none; margin-top: 5px; color: #888; font-style: italic; }

    .admin-link {
        display: inline-block; margin-top: 15px;
        color: #333; font-size: 11px; text-decoration: none;
        border: 1px solid #333; padding: 4px 10px; border-radius: 15px; transition: 0.3s;
    }
    .admin-link:hover { color: #fff; border-color: #fff; background: rgba(255,255,255,0.1); }
  </style>
</head>
<body>
  <div class="card">
    <h2>CLASS ATTENDANCE</h2>
    <form id="attForm">
      <select id="studentSelect" required>
        <option value="" disabled selected>-- Select Your Name --</option>
        %OPTIONS%
      </select>
      <button type="submit" class="btn-main">MARK PRESENT</button>
    </form>

    <div id="msg" style="margin-top:12px; font-size:13px; font-weight:bold;"></div>

    <div class="live-list">
        <div style="font-weight:bold; color:#00d2ff; margin-bottom:8px;">✅ Today's Present List:</div>
        <div id="present-names">Loading...</div>
    </div>

    <button class="btn-sus" onclick="toggleSus()">⚠️ Check Suspicious Activity</button>
    
    <div id="sus-container">
        <div style="font-weight:bold; color:#ff4444; font-size:12px; margin-bottom:5px;">Devices used for multiple students:</div>
        <div id="sus-list">Checking...</div>
    </div>

    <button class="wall-btn" onclick="document.getElementById('bgInput').click()">📷 Change Wallpaper</button>
    <input type="file" id="bgInput" accept="image/*">

    <div class="dev-container">
        <span class="dev-btn" onclick="toggleDev()">Developed By</span>
        <div id="devPanel" class="dev-list">Ajeet Kumar & Reva</div>
    </div>

    <a href="/admin" class="admin-link">🔐 Admin Login</a>
  </div>

  <script>
    const savedBg = localStorage.getItem('class_wall');
    if (savedBg) document.body.style.backgroundImage = `url(${savedBg})`;

    document.getElementById('bgInput').addEventListener('change', function(e) {
      const file = e.target.files[0];
      if (file) {
        const reader = new FileReader();
        reader.onload = function(event) {
          try {
            localStorage.setItem('class_wall', event.target.result);
            document.body.style.backgroundImage = `url(${event.target.result})`;
          } catch(err) { alert("File too large. Try a smaller image."); }
        };
        reader.readAsDataURL(file);
      }
    });

    function toggleDev() {
      var x = document.getElementById("devPanel");
      x.style.display = (x.style.display === "block") ? "none" : "block";
    }

    function toggleSus() {
        var x = document.getElementById("sus-container");
        if(x.style.display === "block") {
            x.style.display = "none";
        } else {
            x.style.display = "block";
            loadSuspicious();
        }
    }

    function loadSuspicious() {
        fetch('/suspicious').then(res => res.json()).then(data => {
            let html = "";
            if(data.length === 0) html = "<span style='color:#888'>No suspicious activity.</span>";
            else data.forEach(n => { html += `<div>❗ ${n}</div>`; });
            document.getElementById('sus-list').innerHTML = html;
        });
    }

    function updateList() {
      fetch('/present').then(res => res.json()).then(data => {
        let html = "";
        if(data.length === 0) html = "<span style='color:#666'>No one yet...</span>";
        data.forEach(n => { html += `<div class='p-item'>⚡ ${n}</div>`; });
        document.getElementById('present-names').innerHTML = html;
      });
    }
    updateList();

    document.getElementById('attForm').onsubmit = function(e) {
      e.preventDefault();
      let sel = document.getElementById('studentSelect');
      let val = sel.value;
      if(!val) return;
      
      document.getElementById('msg').innerHTML = "Wait...";
      let time = new Date().toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'}); 
      
      fetch('/mark?name=' + encodeURIComponent(val) + '&time=' + encodeURIComponent(time))
      .then(res => res.text()).then(text => {
        document.getElementById('msg').innerHTML = text;
        document.getElementById('msg').style.color = text.includes("Success") ? "#00ff88" : "#ff4444";
        if(text.includes("Success")) {
          updateList();
          sel.selectedIndex = 0;
          if(document.getElementById("sus-container").style.display === "block") loadSuspicious();
        }
      });
    };
  </script>
</body>
</html>
)rawliteral";

// ---------------- ADMIN PAGE HTML (New Style) ----------------
const char admin_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Admin Panel</title>
  <style>
    body {
      margin: 0; padding: 0;
      background: #121212 no-repeat center center fixed;
      background-size: cover;
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      min-height: 100vh; display: flex; flex-direction: column;
      justify-content: center; align-items: center; color: white;
    }
    .card {
      width: 85%; max-width: 380px;
      padding: 30px; border-radius: 20px;
      background: rgba(0, 0, 0, 0.75);
      backdrop-filter: blur(12px);
      border: 1px solid rgba(255,255,255,0.1);
      text-align: center; box-shadow: 0 10px 30px rgba(0,0,0,0.5);
    }
    h2 { color: #00d2ff; margin-top: 0; }
    p { color: #aaa; font-size: 13px; }
    
    .btn {
      display: block; width: 100%; padding: 15px; margin: 15px 0;
      color: white; text-decoration: none; border-radius: 10px;
      font-weight: bold; font-size: 15px; border: none; cursor: pointer;
      transition: 0.3s;
    }
    .btn-green { background: linear-gradient(135deg, #28a745, #218838); box-shadow: 0 4px 15px rgba(40,167,69,0.4); }
    .btn-red { background: linear-gradient(135deg, #ff4444, #cc0000); box-shadow: 0 4px 15px rgba(220,53,69,0.4); }
    .btn:active { transform: scale(0.98); }

    .back-link { margin-top: 20px; display: inline-block; color: #888; text-decoration: none; font-size: 13px; }
    .back-link:hover { color: white; }
  </style>
</head>
<body>
  <div class="card">
    <h2>👮 Admin Dashboard</h2>
    <p>Manage attendance records securely.</p>
    
    <a href='/download' class='btn btn-green'>📥 Save Data (CSV)</a>
    
    <a href='/clear' class='btn btn-red' onclick="return confirm('⚠️ Warning: This will DELETE all current attendance data to start a new day. Have you downloaded the CSV?')">🗑️ Start New Class</a>
    
    <a href='/' class="back-link">⬅ Back to Home</a>
  </div>

  <script>
    // Apply same wallpaper as main page
    const savedBg = localStorage.getItem('class_wall');
    if (savedBg) document.body.style.backgroundImage = `url(${savedBg})`;
  </script>
</body>
</html>
)rawliteral";

// ---------------- SERVER HANDLERS ----------------

void handleRoot() {
  String s = FPSTR(index_html);
  String options = "";
  for(int i=0; i<totalStudents; i++){
    options += "<option value='" + students[i] + "'>" + students[i] + "</option>";
  }
  s.replace("%OPTIONS%", options);
  // Send with UTF-8 charset
  server.send(200, "text/html; charset=utf-8", s);
}

struct AttEntry {
  String name;
  String ip;
};

void setup() {
  Serial.begin(115200);
  
  if(!LittleFS.begin()){
    Serial.println("LittleFS Mount Failed");
    return;
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/generate_204", handleRoot);
  server.on("/gstatic", handleRoot);

  server.on("/present", [](){
    String json = "[";
    if(LittleFS.exists("/attendance.csv")){
      File f = LittleFS.open("/attendance.csv", "r");
      bool first = true;
      while(f.available()){
        String line = f.readStringUntil('\n');
        int firstComma = line.indexOf(',');
        if(firstComma > 0) {
          if(!first) json += ",";
          json += "\"" + line.substring(0, firstComma) + "\"";
          first = false;
        }
      }
      f.close();
    }
    json += "]";
    server.send(200, "application/json", json);
  });

  server.on("/suspicious", [](){
    std::vector<AttEntry> entries;
    String json = "[";
    
    if(LittleFS.exists("/attendance.csv")){
      File f = LittleFS.open("/attendance.csv", "r");
      while(f.available()){
        String line = f.readStringUntil('\n');
        int comma1 = line.indexOf(',');
        int comma2 = line.lastIndexOf(',');
        
        if(comma1 > 0 && comma2 > comma1) {
          AttEntry e;
          e.name = line.substring(0, comma1);
          e.ip = line.substring(comma2 + 1); 
          e.ip.trim();
          entries.push_back(e);
        }
      }
      f.close();
    }

    bool first = true;
    for(size_t i = 0; i < entries.size(); i++){
      bool isSuspicious = false;
      for(size_t j = 0; j < entries.size(); j++){
        if(i != j && entries[i].ip == entries[j].ip){
          isSuspicious = true; break;
        }
      }
      if(isSuspicious){
        if(!first) json += ",";
        json += "\"" + entries[i].name + "\""; 
        first = false;
      }
    }
    json += "]";
    server.send(200, "application/json", json);
  });

  server.on("/mark", [](){
    if (server.hasArg("name")) {
      String name = server.arg("name");
      String time = server.arg("time");
      String userIP = server.client().remoteIP().toString(); 
      
      bool exists = false;
      if(LittleFS.exists("/attendance.csv")){
        File r = LittleFS.open("/attendance.csv", "r");
        while(r.available()){
          if(r.readStringUntil('\n').startsWith(name + ",")){ 
            exists = true; break; 
          }
        }
        r.close();
      }

      if(exists){
        server.send(200, "text/plain; charset=utf-8", "⚠️ Pehle se present hain!");
      } else {
        File f = LittleFS.open("/attendance.csv", "a");
        if(f){
          f.println(name + "," + time + "," + userIP);
          f.close();
          server.send(200, "text/plain; charset=utf-8", "✅ Success! Lag gayi.");
        } else {
          server.send(500, "text/plain", "Error: Storage issue.");
        }
      }
    }
  });

  // --- ADMIN HANDLERS (UPDATED) ---
  server.on("/admin", [](){
    if(!server.authenticate(admin_user, admin_pass)) return server.requestAuthentication();
    // Send new Admin HTML with UTF-8
    server.send(200, "text/html; charset=utf-8", FPSTR(admin_html));
  });

  server.on("/download", [](){
    if(!server.authenticate(admin_user, admin_pass)) return server.requestAuthentication();
    File f = LittleFS.open("/attendance.csv", "r");
    server.streamFile(f, "text/csv");
    f.close();
  });

  server.on("/clear", [](){
    if(!server.authenticate(admin_user, admin_pass)) return server.requestAuthentication();
    LittleFS.remove("/attendance.csv");
    // Simple redirect page also with dark theme for consistency
    String msg = "<body style='background:#121212;color:white;text-align:center;font-family:sans-serif;'>";
    msg += "<h2>✅ New Class Started!</h2><a href='/' style='color:#00d2ff'>Go Home</a></body>";
    server.send(200, "text/html", msg);
  });

  server.onNotFound(handleRoot);
  server.begin();
  Serial.println("Server Started");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}