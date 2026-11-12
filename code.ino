/*
 * ESP32 OLED STUDIO v2.0
 * ----------------------------------------------------------------
 * SH1106 128x64 OLED + Wi-Fi access point + futuristic web dashboard
 *
 * Libraries:  Adafruit GFX, Adafruit SH110X, ArduinoJson v7
 * Wiring:     SDA -> GPIO21, SCL -> GPIO22
 * Connect:    Wi-Fi "ESP32-OLED" / "12345678"  ->  http://192.168.4.1
 *
 * Built-in modes : CUBE, BALL, FACE, RADAR, WAVE
 * Custom mode    : upload a JSON program. Plain JSON now ANIMATES:
 *                  every property can tween ("toX", "toY", "toR" ...),
 *                  with easing, typewriter text, blinking, rotating rays,
 *                  progress bars and always-on "static" layers.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ArduinoJson.h>
#include <math.h>

// ======================================================================
// HARDWARE
// ======================================================================
#define SDA_PIN       21
#define SCL_PIN       22
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDR     0x3C
#define W             SH110X_WHITE
#define B             SH110X_BLACK

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ======================================================================
// NETWORK
// ======================================================================
const char* AP_SSID = "ESP32-OLED";
const char* AP_PASS = "12345678";
WebServer server(80);

// ======================================================================
// STATE
// ======================================================================
enum Mode : uint8_t { M_CUBE, M_BALL, M_FACE, M_RADAR, M_WAVE, M_CUSTOM, M_PAUSED };
const char* const MODE_NAMES[] = { "CUBE", "BALL", "FACE", "RADAR", "WAVE", "CUSTOM", "PAUSED" };

Mode mode     = M_CUBE;
Mode prevMode = M_CUBE;

int           speedDelay = 20;      // minimum ms between frames
uint8_t       contrast   = 207;
unsigned long lastFrame  = 0;
unsigned long fpsTimer   = 0;
uint16_t      fpsCount   = 0;
uint16_t      fps        = 0;

// Cube
float angleX = 0, angleY = 0, angleZ = 0;
const float vertices[8][3] = {
  {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
  {-1,-1, 1}, {1,-1, 1}, {1,1, 1}, {-1,1, 1}
};
const uint8_t edges[12][2] = {
  {0,1},{1,2},{2,3},{3,0},
  {4,5},{5,6},{6,7},{7,4},
  {0,4},{1,5},{2,6},{3,7}
};
int px[8], py[8];

// Ball
const int TRAIL = 8;
int ballX = 20, ballY = 20, ballDX = 2, ballDY = 2;
int trailX[TRAIL], trailY[TRAIL];

// Custom animation (kept in RAM)
JsonDocument  animationDoc;
bool          customLoaded    = false;
size_t        customFrame     = 0;
unsigned long customFrameTime = 0;

// ======================================================================
// WEB DASHBOARD
// ======================================================================
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>OLED STUDIO</title>
<style>
:root{--bg:#04070f;--panel:rgba(14,26,46,.72);--line:#1d3d63;--cy:#35e6ff;--mg:#b06cff;--ok:#3dffb0;--bad:#ff5c7a;--tx:#dbeaff;--dim:#7f98b8}
*{box-sizing:border-box}
body{margin:0;min-height:100vh;color:var(--tx);font-family:"Segoe UI",system-ui,Arial,sans-serif;padding:18px;
background:radial-gradient(900px 500px at 85% -10%,rgba(176,108,255,.18),transparent),
radial-gradient(700px 500px at -10% 10%,rgba(53,230,255,.16),transparent),var(--bg)}
body:before{content:"";position:fixed;inset:0;pointer-events:none;opacity:.25;
background-image:linear-gradient(var(--line) 1px,transparent 1px),linear-gradient(90deg,var(--line) 1px,transparent 1px);
background-size:44px 44px;mask-image:linear-gradient(#000,transparent 80%)}
.wrap{position:relative;max-width:720px;margin:auto}
header{display:flex;align-items:center;justify-content:space-between;margin-bottom:6px}
h1{margin:0;font-size:26px;letter-spacing:6px;font-weight:800;background:linear-gradient(90deg,var(--cy),var(--mg));-webkit-background-clip:text;background-clip:text;color:transparent}
.sub{color:var(--dim);font-size:12px;letter-spacing:3px;text-transform:uppercase;margin:0 0 16px}
.badge{display:flex;align-items:center;gap:8px;font-size:12px;letter-spacing:2px;color:var(--dim)}
.dot{width:10px;height:10px;border-radius:50%;background:var(--bad);box-shadow:0 0 10px var(--bad)}
.dot.on{background:var(--ok);box-shadow:0 0 12px var(--ok);animation:pulse 1.6s infinite}
@keyframes pulse{50%{opacity:.35}}
.card{position:relative;background:var(--panel);backdrop-filter:blur(8px);border:1px solid var(--line);border-radius:14px;padding:16px;margin:14px 0}
.card:before,.card:after{content:"";position:absolute;width:14px;height:14px;border:2px solid var(--cy);opacity:.8}
.card:before{top:-1px;left:-1px;border-right:0;border-bottom:0;border-top-left-radius:14px}
.card:after{bottom:-1px;right:-1px;border-left:0;border-top:0;border-bottom-right-radius:14px}
h3{margin:0 0 12px;font-size:12px;letter-spacing:3px;text-transform:uppercase;color:var(--cy)}
.stats{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}
.stat{background:rgba(4,10,20,.6);border:1px solid var(--line);border-radius:10px;padding:10px;text-align:center}
.stat b{display:block;font-size:20px;color:var(--tx);font-family:Consolas,monospace}
.stat span{font-size:10px;letter-spacing:2px;color:var(--dim)}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(104px,1fr));gap:8px}
button{cursor:pointer;color:var(--tx);background:rgba(18,40,70,.8);border:1px solid var(--line);border-radius:9px;padding:12px 8px;
font-weight:700;font-size:12px;letter-spacing:2px;text-transform:uppercase;transition:.18s}
button:hover{border-color:var(--cy);box-shadow:0 0 14px rgba(53,230,255,.35);transform:translateY(-1px)}
button.active{background:linear-gradient(135deg,#0a8fd0,#6a3fd6);border-color:var(--cy);box-shadow:0 0 16px rgba(53,230,255,.5)}
button.go{background:linear-gradient(135deg,#00b894,#0984e3);border:0}
button.sm{padding:8px 10px;font-size:11px}
.row{display:flex;align-items:center;gap:12px;margin:8px 0}
.row label{width:92px;font-size:11px;letter-spacing:2px;color:var(--dim);text-transform:uppercase}
.row output{width:62px;text-align:right;font-family:Consolas,monospace;color:var(--cy)}
input[type=range]{flex:1;accent-color:var(--cy)}
textarea{width:100%;height:230px;resize:vertical;background:#030811;color:#8ff3ff;border:1px solid var(--line);border-radius:10px;padding:12px;font:13px/1.45 Consolas,monospace;outline:0}
textarea:focus{border-color:var(--cy)}
canvas{width:100%;aspect-ratio:2/1;background:#000;border:1px solid var(--line);border-radius:10px;image-rendering:pixelated;box-shadow:inset 0 0 30px rgba(53,230,255,.15)}
#msg{font-size:12px;letter-spacing:1px;min-height:18px;margin-top:8px;font-family:Consolas,monospace}
.ok{color:var(--ok)}.bad{color:var(--bad)}
code{color:#8ff3ff;font-size:12px}
.doc p{margin:5px 0;font-size:13px;color:var(--dim)}
</style>
</head>
<body>
<div class="wrap">

<header>
  <h1>OLED STUDIO</h1>
  <div class="badge"><span class="dot" id="dot"></span><span id="link">LINK</span></div>
</header>
<p class="sub">ESP32 &middot; SH1106 128&times;64 &middot; 192.168.4.1</p>

<div class="card">
  <h3>Telemetry</h3>
  <div class="stats">
    <div class="stat"><b id="mode">--</b><span>MODE</span></div>
    <div class="stat"><b id="fps">--</b><span>FPS</span></div>
    <div class="stat"><b id="heap">--</b><span>FREE HEAP</span></div>
  </div>
</div>

<div class="card">
  <h3>Animation Core</h3>
  <div class="grid">
    <button data-m="cube"  onclick="setMode('cube')">3D Cube</button>
    <button data-m="ball"  onclick="setMode('ball')">Ball</button>
    <button data-m="face"  onclick="setMode('face')">Face</button>
    <button data-m="radar" onclick="setMode('radar')">Radar</button>
    <button data-m="wave"  onclick="setMode('wave')">Wave</button>
    <button data-m="custom" onclick="setMode('custom')">Custom</button>
  </div>
  <div class="grid" style="margin-top:8px">
    <button class="sm" onclick="cmd('/pause')">&#10074;&#10074; Pause</button>
    <button class="sm" onclick="cmd('/resume')">&#9654; Resume</button>
  </div>
</div>

<div class="card">
  <h3>Display Tuning</h3>
  <div class="row"><label>Frame gap</label><input type="range" min="5" max="100" value="20" id="sp" oninput="slide('/speed','sp','spv',' ms')"><output id="spv">20 ms</output></div>
  <div class="row"><label>Brightness</label><input type="range" min="0" max="255" value="207" id="ct" oninput="slide('/contrast','ct','ctv','')"><output id="ctv">207</output></div>
</div>

<div class="card">
  <h3>JSON Animation Studio</h3>
  <canvas id="cv" width="128" height="64"></canvas>
  <div class="grid" style="margin:12px 0">
    <button class="sm" onclick="preset('boot')">Boot</button>
    <button class="sm" onclick="preset('orbit')">Orbit</button>
    <button class="sm" onclick="preset('radar')">Sweep</button>
    <button class="sm" onclick="preset('hello')">Hello</button>
  </div>
  <textarea id="program" spellcheck="false" oninput="load()"></textarea>
  <div id="msg"></div>
  <div class="grid" style="margin-top:10px">
    <button class="go" onclick="upload()">Upload &amp; Run</button>
  </div>
</div>

<div class="card doc">
  <h3>JSON Reference</h3>
  <p><code>frames[]</code> each with <code>duration</code> (ms) and <code>items[]</code>. Optional <code>"loop":false</code> and <code>"static":[]</code> (drawn on every frame).</p>
  <p><b>Types:</b> text, line, circle, fillCircle, rect, fillRect, round, triangle, pixel, ray, bar.</p>
  <p><b>Animate any value</b> by adding its <code>to</code> twin: <code>x &rarr; toX</code>, <code>y &rarr; toY</code>, <code>r &rarr; toR</code>, <code>w &rarr; toW</code>, <code>h &rarr; toH</code>, <code>x2 &rarr; toX2</code>, <code>y2 &rarr; toY2</code>, <code>angle &rarr; toAngle</code>, <code>len &rarr; toLen</code>, <code>value &rarr; toValue</code>.</p>
  <p><b>Extras:</b> <code>"ease"</code>: linear | in | out | inout &middot; <code>"typewriter":true</code> &middot; <code>"blink":300</code> &middot; <code>"size":2</code> &middot; <code>"color":0</code> (erase).</p>
</div>

</div>

<script>
const $=id=>document.getElementById(id);
const PRESETS={
boot:{static:[{type:"rect",x:0,y:0,w:128,h:64}],frames:[
 {duration:1500,items:[{type:"text",x:20,y:10,size:2,value:"SYSTEM",typewriter:true},{type:"bar",x:14,y:42,w:100,h:10,value:0,toValue:100,ease:"inout"}]},
 {duration:900,items:[{type:"text",x:20,y:10,size:2,value:"ONLINE"},{type:"text",x:34,y:40,value:"ALL SYSTEMS GO",blink:300}]}]},
orbit:{static:[{type:"rect",x:0,y:0,w:128,h:64}],frames:[
 {duration:900,items:[{type:"fillCircle",x:10,y:32,toX:118,r:4,toR:8,ease:"inout"},{type:"circle",x:64,y:32,r:20}]},
 {duration:900,items:[{type:"fillCircle",x:118,y:32,toX:10,r:8,toR:4,ease:"inout"},{type:"circle",x:64,y:32,r:20}]}]},
radar:{static:[{type:"circle",x:64,y:32,r:28},{type:"circle",x:64,y:32,r:14},{type:"pixel",x:64,y:32}],frames:[
 {duration:2200,items:[{type:"ray",x:64,y:32,len:28,angle:0,toAngle:360,ease:"linear"},{type:"text",x:2,y:2,value:"SCAN",blink:500}]}]},
hello:{frames:[
 {duration:400,items:[{type:"text",x:10,y:25,value:"HELLO!"},{type:"circle",x:64,y:32,r:12}]},
 {duration:400,items:[{type:"text",x:10,y:25,value:"ESP32"},{type:"rect",x:40,y:15,w:45,h:30}]}]}
};
function preset(n){$('program').value=JSON.stringify(PRESETS[n],null,1);load()}

async function cmd(p){try{await fetch(p);poll()}catch(e){}}
function setMode(m){cmd('/mode?m='+m)}
async function slide(path,id,out,unit){const v=$(id).value;$(out).textContent=v+unit;try{await fetch(path+'?v='+v)}catch(e){}}
function msg(t,c){const m=$('msg');m.textContent=t;m.className=c}

async function poll(){
 try{
  const s=await(await fetch('/api/state')).json();
  $('mode').textContent=s.mode;$('fps').textContent=s.fps;$('heap').textContent=Math.round(s.heap/1024)+' KB';
  $('dot').className='dot on';$('link').textContent='LINKED';
  document.querySelectorAll('[data-m]').forEach(b=>b.classList.toggle('active',b.dataset.m===s.mode.toLowerCase()));
 }catch(e){$('dot').className='dot';$('link').textContent='OFFLINE'}
}

async function upload(){
 load();if(!prog)return;
 try{
  const r=await fetch('/upload',{method:'POST',headers:{'Content-Type':'application/json'},body:$('program').value});
  const t=await r.text();msg(t,r.ok?'ok':'bad');poll();
 }catch(e){msg('Upload failed','bad')}
}

/* ---------- live preview: same engine as the firmware ---------- */
const cv=$('cv'),cx=cv.getContext('2d');
let prog=null,fi=0,ft=0,fin=false;
const EZ={linear:t=>t,in:t=>t*t,out:t=>1-(1-t)*(1-t),inout:t=>t<.5?2*t*t:1-Math.pow(-2*t+2,2)/2};
const tw=(i,k,tk,d,e)=>{const a=i[k]??d;return i[tk]!==undefined?a+(i[tk]-a)*e:a};
function load(){
 try{
  prog=JSON.parse($('program').value);
  if(!Array.isArray(prog.frames)||!prog.frames.length)throw'"frames" array required';
  fi=0;ft=performance.now();fin=false;
  msg('JSON valid · '+prog.frames.length+' frame(s) · '+$('program').value.length+' bytes','ok');
 }catch(e){prog=null;msg('Invalid: '+e,'bad')}
}
function drawItem(i,t){
 const e=(EZ[i.ease]||EZ.inout)(t);
 if(i.blink>0&&Math.floor(performance.now()/i.blink)%2)return;
 const col=(i.color??1)?'#8ff3ff':'#000';
 cx.fillStyle=col;cx.strokeStyle=col;cx.lineWidth=1;
 const x=tw(i,'x','toX',0,e),y=tw(i,'y','toY',0,e);
 const rr=()=>Math.max(0,tw(i,'r','toR',5,e));
 const ww=()=>tw(i,'w','toW',10,e),hh=()=>tw(i,'h','toH',10,e);
 const ln=(a,b,c,d)=>{cx.beginPath();cx.moveTo(a+.5,b+.5);cx.lineTo(c+.5,d+.5);cx.stroke()};
 switch(i.type){
  case'text':{let v=String(i.value??'');if(i.typewriter)v=v.slice(0,Math.round(t*v.length));const s=i.size||1;cx.font=(8*s)+'px monospace';cx.textBaseline='top';cx.fillText(v,x,y);break}
  case'line':ln(x,y,tw(i,'x2','toX2',0,e),tw(i,'y2','toY2',0,e));break;
  case'circle':cx.beginPath();cx.arc(x+.5,y+.5,rr(),0,7);cx.stroke();break;
  case'fillCircle':cx.beginPath();cx.arc(x+.5,y+.5,rr(),0,7);cx.fill();break;
  case'rect':cx.strokeRect(x+.5,y+.5,ww(),hh());break;
  case'fillRect':cx.fillRect(x,y,ww(),hh());break;
  case'round':cx.beginPath();cx.roundRect?cx.roundRect(x+.5,y+.5,ww(),hh(),i.r||3):cx.rect(x+.5,y+.5,ww(),hh());cx.stroke();break;
  case'triangle':{const x2=tw(i,'x2','toX2',0,e),y2=tw(i,'y2','toY2',0,e),x3=tw(i,'x3','toX3',0,e),y3=tw(i,'y3','toY3',0,e);ln(x,y,x2,y2);ln(x2,y2,x3,y3);ln(x3,y3,x,y);break}
  case'pixel':cx.fillRect(x,y,1,1);break;
  case'ray':{const l=tw(i,'len','toLen',10,e),a=tw(i,'angle','toAngle',0,e)*Math.PI/180;ln(x,y,x+Math.cos(a)*l,y+Math.sin(a)*l);break}
  case'bar':{const v=Math.min(100,Math.max(0,tw(i,'value','toValue',0,e))),w=ww(),h=hh();cx.strokeRect(x+.5,y+.5,w,h);cx.fillRect(x+2,y+2,Math.max(0,(w-3)*v/100),Math.max(0,h-3));break}
 }
}
function tick(now){
 cx.fillStyle='#000';cx.fillRect(0,0,128,64);
 if(prog){
  let f=prog.frames[fi],d=Math.max(1,f.duration||200);
  while(!fin&&now-ft>=d){
   if(fi+1>=prog.frames.length){if(prog.loop===false){fin=true;break}fi=0}else fi++;
   ft+=d;f=prog.frames[fi];d=Math.max(1,f.duration||200);
  }
  const t=fin?1:Math.min(1,(now-ft)/d);
  (prog.static||[]).forEach(i=>drawItem(i,t));
  (f.items||[]).forEach(i=>drawItem(i,t));
 }
 requestAnimationFrame(tick);
}
preset('boot');requestAnimationFrame(tick);
setInterval(poll,2000);poll();
</script>
</body>
</html>
)rawliteral";

// ======================================================================
// HELPERS / HUD
// ======================================================================
void setMode(Mode m) {
  mode = m;
  if (m == M_CUSTOM) {
    customFrame     = 0;
    customFrameTime = millis();
  }
}

void drawBrackets() {
  const int L = 6;
  display.drawFastHLine(0, 0, L, W);                              display.drawFastVLine(0, 0, L, W);
  display.drawFastHLine(SCREEN_WIDTH - L, 0, L, W);               display.drawFastVLine(SCREEN_WIDTH - 1, 0, L, W);
  display.drawFastHLine(0, SCREEN_HEIGHT - 1, L, W);              display.drawFastVLine(0, SCREEN_HEIGHT - L, L, W);
  display.drawFastHLine(SCREEN_WIDTH - L, SCREEN_HEIGHT - 1, L, W); display.drawFastVLine(SCREEN_WIDTH - 1, SCREEN_HEIGHT - L, L, W);
}

void drawLabel(const char* label) {
  display.setTextSize(1);
  display.setTextColor(W);
  display.setCursor(5, 5);
  display.print(label);
}

// ======================================================================
// BUILT-IN ANIMATIONS
// ======================================================================
void drawCube() {
  float sx = sinf(angleX), cx = cosf(angleX);
  float sy = sinf(angleY), cy = cosf(angleY);
  float sz = sinf(angleZ), cz = cosf(angleZ);

  for (int i = 0; i < 8; i++) {
    float x = vertices[i][0], y = vertices[i][1], z = vertices[i][2];
    float y1 = y * cx - z * sx;
    float z1 = y * sx + z * cx;
    float x2 = x * cy + z1 * sy;
    float z2 = -x * sy + z1 * cy;
    float x3 = x2 * cz - y1 * sz;
    float y3 = x2 * sz + y1 * cz;
    float p  = 25.0f / (4.0f + z2);
    px[i] = 64 + (int)(x3 * p);
    py[i] = 32 + (int)(y3 * p);
  }

  display.clearDisplay();
  for (int i = 0; i < 12; i++)
    display.drawLine(px[edges[i][0]], py[edges[i][0]], px[edges[i][1]], py[edges[i][1]], W);
  for (int i = 0; i < 8; i++) display.fillCircle(px[i], py[i], 2, W);

  drawBrackets();
  drawLabel("CUBE");
  display.display();

  angleX += 0.035f;
  angleY += 0.025f;
  angleZ += 0.018f;
}

void drawBall() {
  for (int i = TRAIL - 1; i > 0; i--) { trailX[i] = trailX[i - 1]; trailY[i] = trailY[i - 1]; }
  trailX[0] = ballX; trailY[0] = ballY;

  ballX += ballDX;
  ballY += ballDY;
  if (ballX <= 8 || ballX >= 119) ballDX = -ballDX;
  if (ballY <= 8 || ballY >= 55)  ballDY = -ballDY;

  display.clearDisplay();
  for (int i = TRAIL - 1; i >= 1; i--) display.fillCircle(trailX[i], trailY[i], (TRAIL - i) / 3, W);
  display.drawCircle(ballX, ballY, 5, W);
  display.fillCircle(ballX, ballY, 2, W);

  drawBrackets();
  drawLabel("BALL");
  display.display();
}

void drawFace() {
  unsigned long ms = millis();
  bool blink = (ms % 3200) < 150;
  bool talk  = ((ms / 450) % 4) == 0;
  int  look  = (int)(sinf(ms / 900.0f) * 2.0f);

  display.clearDisplay();
  display.drawCircle(64, 32, 25, W);

  if (blink) {
    display.drawFastHLine(51 + look, 25, 8, W);
    display.drawFastHLine(69 + look, 25, 8, W);
  } else {
    display.fillCircle(55 + look, 25, 3, W);
    display.fillCircle(73 + look, 25, 3, W);
  }

  if (talk) {
    display.drawRoundRect(57, 40, 14, 8, 3, W);
  } else {
    for (int x = -11; x <= 11; x++) display.drawPixel(64 + x, 42 + (x * x) / 30, W);
  }

  drawBrackets();
  drawLabel("FACE");
  display.display();
}

void drawRadar() {
  const int cx = 64, cy = 32;
  float a = fmodf(millis() / 700.0f, TWO_PI);

  display.clearDisplay();
  display.drawCircle(cx, cy, 30, W);
  display.drawCircle(cx, cy, 20, W);
  display.drawCircle(cx, cy, 10, W);
  for (int i = -30; i <= 30; i += 4) { display.drawPixel(cx + i, cy, W); display.drawPixel(cx, cy + i, W); }

  for (int k = 0; k < 6; k += 2) {
    float t = a - k * 0.09f;
    display.drawLine(cx, cy, cx + (int)(cosf(t) * 30), cy + (int)(sinf(t) * 30), W);
  }

  const float targetA[3] = {0.9f, 2.7f, 4.6f};
  const float targetR[3] = {22, 14, 26};
  for (int i = 0; i < 3; i++) {
    float diff = fmodf(a - targetA[i] + TWO_PI * 2, TWO_PI);
    if (diff < 2.2f) {
      int s = diff < 0.8f ? 3 : (diff < 1.5f ? 2 : 1);
      display.fillCircle(cx + (int)(cosf(targetA[i]) * targetR[i]), cy + (int)(sinf(targetA[i]) * targetR[i]), s, W);
    }
  }

  drawBrackets();
  drawLabel("RADAR");
  display.display();
}

void drawWave() {
  float ph = millis() / 400.0f;
  int prev1 = 32, prev2 = 32;

  display.clearDisplay();
  for (int x = 0; x < SCREEN_WIDTH; x++) {
    int y1 = 32 + (int)(14.0f * sinf(x * 0.08f + ph) * sinf(x * 0.02f + ph * 0.3f));
    int y2 = 32 + (int)(8.0f * sinf(x * 0.15f - ph * 1.5f));
    if (x > 0) {
      display.drawLine(x - 1, prev1, x, y1, W);
      if (x % 2 == 0) display.drawPixel(x, y2, W);
    }
    prev1 = y1; prev2 = y2;
  }
  (void)prev2;

  drawBrackets();
  drawLabel("WAVE");
  display.display();
}

// ======================================================================
// CUSTOM JSON ANIMATION ENGINE
// ======================================================================
float easeFn(float t, const char* name) {
  if (t < 0) t = 0;
  if (t > 1) t = 1;
  if (!strcmp(name, "linear")) return t;
  if (!strcmp(name, "in"))     return t * t;
  if (!strcmp(name, "out"))    return 1.0f - (1.0f - t) * (1.0f - t);
  return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;   // inout (default)
}

// Value of property `key`; if `toKey` exists it is tweened by eased progress e.
float tw(JsonObject it, const char* key, const char* toKey, float def, float e) {
  float a = it[key] | def;
  if (it[toKey].isNull()) return a;
  float b = it[toKey] | a;
  return a + (b - a) * e;
}

void drawItem(JsonObject it, float t) {
  int blink = it["blink"] | 0;
  if (blink > 0 && ((millis() / blink) & 1)) return;

  const char* type = it["type"] | "";
  float e = easeFn(t, it["ease"] | "inout");
  uint16_t col = (it["color"] | 1) ? W : B;

  int x = lroundf(tw(it, "x", "toX", 0, e));
  int y = lroundf(tw(it, "y", "toY", 0, e));

  if (!strcmp(type, "text")) {
    const char* val = it["value"] | "";
    display.setTextSize(it["size"] | 1);
    display.setTextColor(col);
    display.setCursor(x, y);
    if (it["typewriter"] | false) {
      char buf[64];
      size_t len   = strlen(val);
      size_t shown = (size_t)(t * len + 0.5f);
      if (shown > len) shown = len;
      if (shown > sizeof(buf) - 1) shown = sizeof(buf) - 1;
      memcpy(buf, val, shown);
      buf[shown] = 0;
      display.print(buf);
    } else {
      display.print(val);
    }
  }
  else if (!strcmp(type, "line")) {
    display.drawLine(x, y, lroundf(tw(it, "x2", "toX2", 0, e)), lroundf(tw(it, "y2", "toY2", 0, e)), col);
  }
  else if (!strcmp(type, "circle")) {
    display.drawCircle(x, y, lroundf(tw(it, "r", "toR", 5, e)), col);
  }
  else if (!strcmp(type, "fillCircle")) {
    display.fillCircle(x, y, lroundf(tw(it, "r", "toR", 5, e)), col);
  }
  else if (!strcmp(type, "rect")) {
    display.drawRect(x, y, lroundf(tw(it, "w", "toW", 10, e)), lroundf(tw(it, "h", "toH", 10, e)), col);
  }
  else if (!strcmp(type, "fillRect")) {
    display.fillRect(x, y, lroundf(tw(it, "w", "toW", 10, e)), lroundf(tw(it, "h", "toH", 10, e)), col);
  }
  else if (!strcmp(type, "round")) {
    display.drawRoundRect(x, y, lroundf(tw(it, "w", "toW", 10, e)), lroundf(tw(it, "h", "toH", 10, e)), it["r"] | 3, col);
  }
  else if (!strcmp(type, "triangle")) {
    int x2 = lroundf(tw(it, "x2", "toX2", 0, e)), y2 = lroundf(tw(it, "y2", "toY2", 0, e));
    int x3 = lroundf(tw(it, "x3", "toX3", 0, e)), y3 = lroundf(tw(it, "y3", "toY3", 0, e));
    display.drawTriangle(x, y, x2, y2, x3, y3, col);
  }
  else if (!strcmp(type, "pixel")) {
    display.drawPixel(x, y, col);
  }
  else if (!strcmp(type, "ray")) {
    float len = tw(it, "len", "toLen", 10, e);
    float ang = tw(it, "angle", "toAngle", 0, e) * DEG_TO_RAD;
    display.drawLine(x, y, x + lroundf(cosf(ang) * len), y + lroundf(sinf(ang) * len), col);
  }
  else if (!strcmp(type, "bar")) {
    int w = lroundf(tw(it, "w", "toW", 60, e));
    int h = lroundf(tw(it, "h", "toH", 8, e));
    float v = constrain(tw(it, "value", "toValue", 0, e), 0.0f, 100.0f);
    display.drawRect(x, y, w, h, col);
    display.fillRect(x + 2, y + 2, (int)((w - 4) * v / 100.0f), h - 4 > 0 ? h - 4 : 0, col);
  }
}

void drawCustom() {
  JsonArray frames = animationDoc["frames"];
  size_t n = frames.size();
  if (n == 0) { setMode(M_CUBE); return; }

  bool loop = animationDoc["loop"] | true;
  if (customFrame >= n) customFrame = 0;

  unsigned long now = millis();
  JsonObject frame  = frames[customFrame].as<JsonObject>();
  unsigned long dur = frame["duration"] | 200;
  if (dur < 1) dur = 1;

  bool finished = false;
  while (now - customFrameTime >= dur) {
    if (customFrame + 1 >= n) {
      if (!loop) { finished = true; break; }
      customFrame = 0;
    } else {
      customFrame++;
    }
    customFrameTime += dur;
    frame = frames[customFrame].as<JsonObject>();
    dur = frame["duration"] | 200;
    if (dur < 1) dur = 1;
  }

  float t = finished ? 1.0f : (float)(now - customFrameTime) / (float)dur;

  display.clearDisplay();

  JsonArray statics = animationDoc["static"];
  for (JsonObject it : statics) drawItem(it, t);

  JsonArray items = frame["items"];
  for (JsonObject it : items) drawItem(it, t);

  display.display();
}

// ======================================================================
// WEB SERVER ROUTES
// ======================================================================
bool modeFromName(const String& s, Mode& out) {
  if (s == "cube")  { out = M_CUBE;  return true; }
  if (s == "ball")  { out = M_BALL;  return true; }
  if (s == "face")  { out = M_FACE;  return true; }
  if (s == "radar") { out = M_RADAR; return true; }
  if (s == "wave")  { out = M_WAVE;  return true; }
  if (s == "custom" && customLoaded) { out = M_CUSTOM; return true; }
  return false;
}

void setupServer() {

  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html", webpage); });

  server.on("/mode", HTTP_GET, []() {
    Mode m;
    if (server.hasArg("m") && modeFromName(server.arg("m"), m)) {
      setMode(m);
      server.send(200, "text/plain", String("Mode: ") + MODE_NAMES[mode]);
    } else {
      server.send(400, "text/plain", "Unknown mode (or no custom animation uploaded yet)");
    }
  });

  server.on("/pause", HTTP_GET, []() {
    if (mode != M_PAUSED) prevMode = mode;
    mode = M_PAUSED;
    server.send(200, "text/plain", "Paused");
  });

  server.on("/resume", HTTP_GET, []() {
    if (mode == M_PAUSED) mode = prevMode;
    server.send(200, "text/plain", "Resumed");
  });

  server.on("/speed", HTTP_GET, []() {
    if (server.hasArg("v")) speedDelay = constrain(server.arg("v").toInt(), 5, 100);
    server.send(200, "text/plain", "Speed updated");
  });

  server.on("/contrast", HTTP_GET, []() {
    if (server.hasArg("v")) {
      contrast = (uint8_t)constrain(server.arg("v").toInt(), 0, 255);
      display.setContrast(contrast);
    }
    server.send(200, "text/plain", "Brightness updated");
  });

  server.on("/api/state", HTTP_GET, []() {
    String j = "{\"mode\":\"";
    j += MODE_NAMES[mode];
    j += "\",\"fps\":";      j += fps;
    j += ",\"speed\":";      j += speedDelay;
    j += ",\"contrast\":";   j += contrast;
    j += ",\"custom\":";     j += customLoaded ? "true" : "false";
    j += ",\"heap\":";       j += ESP.getFreeHeap();
    j += "}";
    server.send(200, "application/json", j);
  });

  server.on("/upload", HTTP_POST, []() {
    if (!server.hasArg("plain")) {
      server.send(400, "text/plain", "No JSON received");
      return;
    }
    if (server.arg("plain").length() > 8000) {
      server.send(413, "text/plain", "Program too large (8 KB max)");
      return;
    }

    JsonDocument newDoc;
    DeserializationError error = deserializeJson(newDoc, server.arg("plain"));
    if (error) {
      server.send(400, "text/plain", "Invalid JSON: " + String(error.c_str()));
      return;
    }
    if (!newDoc["frames"].is<JsonArray>() || newDoc["frames"].size() == 0) {
      server.send(400, "text/plain", "JSON must contain a non-empty frames array");
      return;
    }

    animationDoc.clear();
    animationDoc.set(newDoc);
    customLoaded = true;
    setMode(M_CUSTOM);

    server.send(200, "text/plain", "Uploaded: " + String(newDoc["frames"].size()) + " frame(s) running");
  });

  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
}

// ======================================================================
// BOOT SPLASH
// ======================================================================
void bootSplash() {
  for (int p = 0; p <= 100; p += 4) {
    display.clearDisplay();
    drawBrackets();
    display.setTextSize(2);
    display.setTextColor(W);
    display.setCursor(14, 12);
    display.print("OLED");
    display.setCursor(14, 30);
    display.print("STUDIO");
    display.drawRect(14, 52, 100, 6, W);
    display.fillRect(16, 54, p - 4 > 0 ? p - 4 : 0, 2, W);
    display.display();
    delay(18);
  }
}

// ======================================================================
// SETUP / LOOP
// ======================================================================
void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("OLED initialization failed!");
    while (true) delay(100);
  }
  display.setContrast(contrast);
  bootSplash();

  for (int i = 0; i < TRAIL; i++) { trailX[i] = ballX; trailY[i] = ballY; }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS, 1, 0, 4);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  setupServer();
  setMode(M_CUBE);
  Serial.println("OLED Studio ready");
}

void loop() {
  server.handleClient();

  unsigned long now = millis();
  if (now - lastFrame < (unsigned long)speedDelay) return;
  lastFrame = now;

  if (mode != M_PAUSED) fpsCount++;
  if (now - fpsTimer >= 1000) {
    fps = fpsCount;
    fpsCount = 0;
    fpsTimer = now;
  }

  switch (mode) {
    case M_CUBE:   drawCube();   break;
    case M_BALL:   drawBall();   break;
    case M_FACE:   drawFace();   break;
    case M_RADAR:  drawRadar();  break;
    case M_WAVE:   drawWave();   break;
    case M_CUSTOM: drawCustom(); break;
    case M_PAUSED: break;
  }
}
