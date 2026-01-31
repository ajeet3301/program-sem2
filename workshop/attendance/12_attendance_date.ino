#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

// ---------------- CONFIGURATION ----------------
const char* ssid = "Class_Attendance";       
const char* password = "password123";       
const char* admin_user = "teacher";         
const char* admin_pass = "admin123";         

String classCode = "ICS";
String classDate = "20260129";
String subjectCode = "CS";

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
  <meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Class Attendance</title>
  <style>
    html, body { height: 100%; margin: 0; padding: 0; }
    body { background: #121212 no-repeat center center fixed; background-size: cover; font-family: 'Segoe UI', sans-serif; display: flex; flex-direction: column; justify-content: center; align-items: center; color: white; }
    .card { width: 85%; max-width: 380px; padding: 25px; border-radius: 20px; background: rgba(0, 0, 0, 0.75); backdrop-filter: blur(12px); border: 1px solid rgba(255,255,255,0.1); text-align: center; box-shadow: 0 10px 30px rgba(0,0,0,0.5); }
    h2 { color: #00d2ff; margin: 0; }
    .info-tag { font-size: 11px; color: #888; margin-bottom: 15px; }
    select { width: 100%; padding: 12px; margin-bottom: 15px; background: #1a1a1a; border: 1px solid #444; border-radius: 10px; color: #fff; outline: none; }
    .btn-main { width: 100%; padding: 13px; border-radius: 10px; border: none; background: linear-gradient(135deg, #00d2ff, #3a7bd5); color: white; font-weight: bold; cursor: pointer; }
    .live-list { margin-top: 20px; text-align: left; border-top: 1px solid #333; padding-top: 15px; }
    #present-names { max-height: 120px; overflow-y: auto; font-size: 13px; color: #ccc; }
    
    /* Sequence Styles */
    .undo-btn { display: block; margin: 15px auto 8px auto; background: none; border: none; color: #ff4444; font-size: 11px; cursor: pointer; text-decoration: underline; opacity: 0.8; }
    .wall-btn { background: none; border: none; color: #666; font-size: 11px; margin-bottom: 8px; cursor: pointer; text-decoration: underline; }
    .admin-link { display: inline-block; color: #444; font-size: 10px; text-decoration: none; border: 1px solid #333; padding: 3px 8px; border-radius: 15px; margin-bottom: 15px; }
    .dev-container { font-size: 11px; color: #555; border-top: 1px solid #222; padding-top: 10px; }
    .dev-list { display: none; margin-top: 5px; color: #888; font-style: italic; }
  </style>
</head>
<body>
  <div class="card">
    <h2>CLASS ATTENDANCE</h2>
    <div class="info-tag">%CLASS_INFO%</div>
    <form id="attForm">
      <select id="studentSelect" required><option value="" disabled selected>-- Select Your Name --</option>%OPTIONS%</select>
      <button type="submit" class="btn-main">MARK PRESENT</button>
    </form>
    <div id="msg" style="margin-top:10px; font-weight:bold; font-size:13px;"></div>
    <div class="live-list">
        <div id="list-header" style="font-weight:bold; color:#00d2ff; margin-bottom:8px;">✅ Present List: 0 / %TOTAL%</div>
        <div id="present-names">Loading...</div>
    </div>
    
    <button class="undo-btn" onclick="undoAttendance()">✖ Mistake? Remove my attendance</button>
    <button class="wall-btn" onclick="document.getElementById('bgInput').click()"> Change Wallpaper</button>
    <input type="file" id="bgInput" style="display:none" accept="image/*">
    <br>
    <div class="dev-container">
        <a href="/admin" class="admin-link"> Admin Login</a> <br>
        <span onclick="toggleDev()" style="cursor:pointer">Developed By</span>
        <div id="devPanel" class="dev-list">Ajeet Kumar</div>
    </div>
  </div>
  <script>
    const savedBg = localStorage.getItem('class_wall');
    if (savedBg) document.body.style.backgroundImage = `url(${savedBg})`;
    document.getElementById('bgInput').onchange = e => {
      const reader = new FileReader();
      reader.onload = ev => { localStorage.setItem('class_wall', ev.target.result); document.body.style.backgroundImage = `url(${ev.target.result})`; };
      reader.readAsDataURL(e.target.files[0]);
    };
    function toggleDev() { var x = document.getElementById("devPanel"); x.style.display = (x.style.display === "block") ? "none" : "block"; }
    function updateList() { fetch('/present').then(res => res.json()).then(data => {
      document.getElementById('list-header').innerHTML = `✅ Present List: ${data.count} / ${data.total}`;
      document.getElementById('present-names').innerHTML = data.names.map(n => `<div> ${n}</div>`).join('') || "No one yet";
    }); }
    updateList();
    document.getElementById('attForm').onsubmit = e => {
      e.preventDefault();
      let name = document.getElementById('studentSelect').value;
      let time = new Date().toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'});
      fetch(`/mark?name=${encodeURIComponent(name)}&time=${time}`).then(res => res.text()).then(t => {
        document.getElementById('msg').innerHTML = t;
        document.getElementById('msg').style.color = t.includes("Success") ? "#00ff88" : "#ff4444";
        updateList();
      });
    };
    function undoAttendance() { if(confirm("Hata dein?")) fetch('/undo').then(res => res.text()).then(t => { alert(t); updateList(); }); }
  </script>
</body>
</html>
)rawliteral";

// ---------------- ADMIN PAGE HTML ----------------
const char admin_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Admin Panel</title>
  <style>
    html, body { height: 100%; margin: 0; padding: 0; }
    body { background: #121212 no-repeat center center fixed; background-size: cover; font-family: 'Segoe UI', sans-serif; display: flex; justify-content: center; align-items: center; color: white; }
    .card { background: rgba(0,0,0,0.75); padding: 30px; border-radius: 20px; width: 320px; border: 1px solid rgba(255,255,255,0.1); backdrop-filter: blur(12px); text-align: center; box-shadow: 0 10px 30px rgba(0,0,0,0.5); }
    input { width: 90%; padding: 10px; margin: 8px 0; background: #1a1a1a; border: 1px solid #444; color: #fff; border-radius: 8px; outline: none; }
    .btn { display: block; width: 100%; padding: 12px; margin: 10px 0; border-radius: 10px; font-weight: bold; border: none; cursor: pointer; text-decoration: none; color: white; }
    .btn-green { background: linear-gradient(135deg, #28a745, #218838); } .btn-red { background: linear-gradient(135deg, #ff4444, #cc0000); }
  </style>
</head>
<body>
  <div class="card">
    <h2>👮 Admin Panel</h2>
    <a href='/download' class='btn btn-green'>📥 Download CSV</a>
    <hr style="border:0.5px solid #333">
    <h4>Start New Class</h4>
    <form action="/clear">
      <input type="text" name="cc" placeholder="Class Code (ICS)" required>
      <input type="text" name="date" placeholder="Date (20260129)" required>
      <input type="text" name="sc" placeholder="Subject Code (CS)" required>
      <button type="submit" class="btn btn-red">🗑️ Reset & Start</button>
    </form>
    <a href='/' style="color:#666; font-size:12px; text-decoration:none;">⬅ Back</a>
  </div>
  <script>
    const savedBg = localStorage.getItem('class_wall');
    if (savedBg) document.body.style.backgroundImage = `url(${savedBg})`;
  </script>
</body>
</html>
)rawliteral";

// ---------------- SERVER FUNCTIONS (Place before setup) ----------------

void handleRoot() {
  String s = FPSTR(index_html);
  String options = "";
  for(int i=0; i<totalStudents; i++) options += "<option value='" + students[i] + "'>" + students[i] + "</option>";
  s.replace("%OPTIONS%", options);
  s.replace("%TOTAL%", String(totalStudents));
  s.replace("%CLASS_INFO%", classCode + " | " + classDate + " | " + subjectCode);
  server.send(200, "text/html; charset=utf-8", s);
}

void handlePresent() {
  String namesArray = ""; int count = 0;
  if(LittleFS.exists("/attendance.csv")){
    File f = LittleFS.open("/attendance.csv", "r");
    while(f.available()){
      String line = f.readStringUntil('\n');
      int c1 = line.indexOf(',');
      if(c1 > 0) {
        if(count > 0) namesArray += ",";
        namesArray += "\"" + line.substring(0, c1) + "\"";
        count++;
      }
    }
    f.close();
  }
  server.send(200, "application/json", "{\"count\":"+String(count)+",\"total\":"+String(totalStudents)+",\"names\":["+namesArray+"]}");
}

void handleMark() {
  String name = server.arg("name");
  String time = server.arg("time");
  String ip = server.client().remoteIP().toString();
  File r = LittleFS.open("/attendance.csv", "r");
  while(r.available()){
    String line = r.readStringUntil('\n');
    if(line.indexOf(name) >= 0) { server.send(200, "text/plain", "⚠️ Pehle se present hain!"); r.close(); return; }
    if(line.indexOf(ip) >= 0) { server.send(200, "text/plain", "❌ Proxy Alert!"); r.close(); return; }
  }
  r.close();
  File f = LittleFS.open("/attendance.csv", "a");
  f.println(name + "," + time + "," + ip);
  f.close();
  server.send(200, "text/plain", "✅ Success!");
}

void handleUndo() {
  String ip = server.client().remoteIP().toString();
  File f = LittleFS.open("/attendance.csv", "r");
  File temp = LittleFS.open("/temp.csv", "w");
  bool found = false;
  while(f.available()){
    String line = f.readStringUntil('\n');
    if(line.indexOf(ip) == -1) temp.println(line);
    else found = true;
  }
  f.close(); temp.close();
  LittleFS.remove("/attendance.csv");
  LittleFS.rename("/temp.csv", "/attendance.csv");
  server.send(200, "text/plain", found ? "Attendance Removed!" : "Not found.");
}

void handleDownload() {
  if(!server.authenticate(admin_user, admin_pass)) return server.requestAuthentication();
  File f = LittleFS.open("/attendance.csv", "r");
  String filename = classCode + classDate + subjectCode + ".csv";
  server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
  server.streamFile(f, "text/csv");
  f.close();
}

void handleClear() {
  if(!server.authenticate(admin_user, admin_pass)) return server.requestAuthentication();
  if(server.hasArg("cc")) classCode = server.arg("cc");
  if(server.hasArg("date")) classDate = server.arg("date");
  if(server.hasArg("sc")) subjectCode = server.arg("sc");
  LittleFS.remove("/attendance.csv");
  server.send(200, "text/html", "<body style='background:#121212;color:white;text-align:center;'><h2>Reset Success!</h2><a href='/'>Go Home</a></body>");
}

void handleAdmin() {
  if(!server.authenticate(admin_user, admin_pass)) return server.requestAuthentication();
  server.send(200, "text/html", FPSTR(admin_html));
}

// ---------------- SETUP & LOOP ----------------

void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  WiFi.softAP(ssid, password);
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/present", handlePresent);
  server.on("/mark", handleMark);
  server.on("/undo", handleUndo);
  server.on("/download", handleDownload);
  server.on("/clear", handleClear);
  server.on("/admin", handleAdmin);
  server.onNotFound(handleRoot);
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

}
