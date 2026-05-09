#pragma once

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>PandaDeck Dash</title>
<link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>
<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.2/css/all.min.css'>
<style>
body{background:#121212;color:white}
.card{background:#1e1e1e;border:1px solid #333;color:white;margin-bottom:15px}
.btn-grid{display:grid;grid-template-columns:repeat(auto-fill, minmax(200px, 1fr));gap:15px}
.btn-del{padding:0 5px;color:#ff4444;cursor:pointer;border:none;background:none}
.hidden-card{display:none}
.icon-select{font-family:'Font Awesome 6 Free','FontAwesome',sans-serif;font-weight:900}
.combo-builder{background:#2a2a2a;border-radius:4px;padding:5px;margin-top:5px;border:1px solid #444}
</style>
</head>
<body class='container py-4'>
<div class='d-flex justify-content-between align-items-center mb-4'>
<h2>PandaDeck Dash <span class='badge bg-secondary' style='font-size:0.5em'>v__VERSION__</span></h2>
<div class='d-flex align-items-center gap-3'>
<div class='d-flex align-items-center gap-2'><label id="lblKb">Keyboard:</label><select id='langSelect' class='form-select form-select-sm' style='width:105px'><option value='0'>English</option><option value='1'>Espanol</option></select></div>
<div class='d-flex align-items-center gap-2'><label id="lblOs">OS:</label><select id='osSelect' class='form-select form-select-sm' style='width:105px'><option value='0'>Windows</option><option value='1'>macOS</option></select></div>
<div class='d-flex align-items-center gap-2'><label id="lblGrid">Grid:</label><select id='gridSelect' class='form-select form-select-sm' style='width:100px'><option value='2x2'>2x2</option><option value='3x2'>3x2</option><option value='3x3'>3x3</option><option value='4x3'>4x3</option><option value='5x3'>5x3</option></select></div>
<div class='d-flex align-items-center gap-2'><label id="lblBg">Background:</label><input type='color' id='globalBg' name='bg' form='configForm' class='form-control form-control-color' style='height:35px'></div>
</div></div>

<div class='row'><div class='col-md-9'>
<div class='card p-3 mb-4'><h5 id="lblBtnConfig">Button Configuration</h5>
<form id='configForm'><div class='btn-grid' id='buttonContainer'></div>
<button type='submit' class='btn btn-primary mt-3 w-100' id="lblSaveChanges">Save Changes</button></form></div></div>

<div class='col-md-3'>
<div class='card p-3 mb-3'><h5 id="lblBackup">Backup & Restore</h5>
<button onclick='backup()' class='btn btn-sm btn-info w-100 mb-2' id="lblDownloadBackup">Download Backup</button>
<input type='file' id='restoreInput' class='form-control form-control-sm mb-2' accept='.json'>
<button onclick='restore()' class='btn btn-sm btn-danger w-100' id="lblRestoreBackup">Restore Backup</button></div>

<div class='card p-3 mb-3'><h5 id="lblFirmware">Firmware OTA</h5>
<p class='small text-secondary' id="lblFirmwareInfo">Select .bin file to update the device.</p>
<input type='file' id='otaInput' class='form-control form-control-sm mb-2' accept='.bin'>
<button onclick='updateFirmware()' class='btn btn-sm btn-warning w-100' id="lblUpdate">Update</button>
<div style='display:none;margin-top:10px' id='otaProgressContainer'>
<div class='progress' style='height:20px'><div id='otaProgressBar' class='progress-bar progress-bar-striped progress-bar-animated bg-warning' style='width:0%'></div></div>
<div id='otaProgressStatus' style='text-align:center;margin-top:5px;font-weight:bold;color:#888'>0%</div></div></div>

<div class='card p-3'><h5 id="lblLibrary">Library</h5>
<input type='file' id='fileInput' class='form-control form-control-sm mb-2'>
<button onclick='upload()' class='btn btn-sm btn-success w-100 mb-3' id="lblUpload">Upload</button>
<ul id='fileList' class='list-group list-group-flush small'></ul></div>
</div></div></div>

<script>
const SYMBOLS={"None":"","OK":"\uF00C","Close":"\uF00D","Copy":"\uF0C5","Paste":"\uF0EA","Cut":"\uF0C4","Play":"\uF04B","Pause":"\uF04C","PlayPause":"\uF04B\uF04C","Mute":"\uF026","Settings":"\uF013","Home":"\uF015","Save":"\uF0C7","Edit":"\uF304","File":"\uF15B","Dir":"\uF07B","Plus":"\uF067","Prev":"\uF048","Next":"\uF051","Stop":"\uF04D"};
const KEYS=['','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','0','1','2','3','4','5','6','7','8','9','F1','F2','F3','F4','F5','F6','F7','F8','F9','F10','F11','F12','ENTER','SPACE','TAB','ESC','UP','DOWN','LEFT','RIGHT','HOME','END','PAGE_UP','PAGE_DOWN','BACKSPACE','DELETE','PRINT_SCREEN','PAUSE'];

let L10N = {};

function toggleBuilder(i){
 const t=document.getElementById('type'+i).value;
 document.getElementById('builder'+i).classList.toggle('d-none',t!='3');
 document.getElementById('basicHint'+i).classList.toggle('d-none',t!='2');
 if(t=='3') updC(i);
}

function updC(i){
 let c='';
 if(document.getElementById('c'+i).checked) c+='CTRL+';
 if(document.getElementById('s'+i).checked) c+='SHIFT+';
 if(document.getElementById('a'+i).checked) c+='ALT+';
 if(document.getElementById('m'+i).checked) c+='GUI+';
 const k=document.getElementById('key'+i).value;
 if(k) c+=k; else if(c.endsWith('+')) c=c.slice(0,-1);
 document.getElementById('val'+i).value=c;
}

function parseC(i,v){
 if(!v)return;
 const p=v.toUpperCase().split('+');
 document.getElementById('c'+i).checked=p.includes('CTRL');
 document.getElementById('s'+i).checked=p.includes('SHIFT');
 document.getElementById('a'+i).checked=p.includes('ALT');
 document.getElementById('m'+i).checked=p.includes('GUI')||p.includes('WIN')||p.includes('CMD');
 const k=p.find(x=>!['CTRL','SHIFT','ALT','GUI','WIN','CMD'].includes(x))||'';
 document.getElementById('key'+i).value=k;
}

function updateVisibleCards(r,c){
 const count=r*c;
 for(let i=0;i<20;i++){
  const card=document.getElementById('card'+i);
  if(card) card.style.display=(i<count)?'block':'none';
 }
}

async function load(){
 try{
  const r=await fetch('/api/config');const d=await r.json();
  const lr=await fetch('/api/l10n');const l=await lr.json();L10N=l;
  document.getElementById('globalBg').value='#'+d.bg.padStart(6,'0');
  document.getElementById('gridSelect').value=d.cols+'x'+d.rows;
  document.getElementById('osSelect').value=d.os;
  document.getElementById('langSelect').value=d.lang;
  updateVisibleCards(d.rows,d.cols);
  const fr=await fetch('/api/files');const files=await fr.json();
  const selects=document.querySelectorAll('.asset-select');
  selects.forEach(s=>{s.innerHTML='<option value="">'+l.none+'</option>';files.forEach(file=>s.innerHTML+=`<option value='${file.name}'>${file.name}</option>`)});
  for(let i=0;i<20;i++){
   const s=document.getElementById('key'+i);
   s.innerHTML=KEYS.map(k=>`<option value='${k}'>${k||l.select_key_ph}</option>`).join('');
  }
  for(let i=0;i<20;i++){
   if(!document.getElementsByName('b'+i+'l')[0]) continue;
   document.getElementsByName('b'+i+'l')[0].value=d.buttons[i].label;
   document.getElementsByName('b'+i+'v')[0].value=d.buttons[i].value;
   document.getElementsByName('b'+i+'t')[0].value=d.buttons[i].type;
   document.getElementsByName('b'+i+'c')[0].value='#'+d.buttons[i].color.padStart(6,'0');
   const sIcon=document.getElementsByName('b'+i+'icon')[0];
    sIcon.innerHTML=Object.entries(SYMBOLS).map(([name,ch])=>`<option value='${name}'>${ch?ch+' ':''}${name}</option>`).join('');
   sIcon.value=d.buttons[i].icon||'None';
   document.getElementsByName('b'+i+'i')[0].value=d.buttons[i].img.startsWith('/')?d.buttons[i].img.substring(1):d.buttons[i].img;
   parseC(i,d.buttons[i].value);toggleBuilder(i);
  }
  const fl=document.getElementById('fileList');fl.innerHTML='';
  files.forEach(file=>fl.innerHTML+=`<li class='list-group-item bg-dark text-white d-flex justify-content-between align-items-center px-2' style='border-color:#333'>${file.name} <button onclick="del('${file.name}')" class='btn-del'>&times;</button></li>`);
 }catch(e){console.error(e)}
}

document.getElementById('osSelect').onchange=async(e)=>{
 document.getElementById('osInput').value=e.target.value;
 const fd=new FormData();fd.append('os',e.target.value);
 await fetch('/api/save',{method:'POST',body:fd});
 load();
};

document.getElementById('langSelect').onchange=async(e)=>{
 const fd=new FormData();fd.append('lang',e.target.value);
 await fetch('/api/save',{method:'POST',body:fd});
 location.reload();
};

document.getElementById('gridSelect').onchange=(e)=>{
 const[c,r]=e.target.value.split('x').map(Number);
 document.getElementById('rowsInput').value=r;
 document.getElementById('colsInput').value=c;
 updateVisibleCards(r,c);
};

document.getElementById('configForm').onsubmit=async(e)=>{
 e.preventDefault();const fd=new FormData(e.target);
 await fetch('/api/save',{method:'POST',body:fd});
 alert(L10N.config_saved);load();
};

async function upload(){
 const fi=document.getElementById('fileInput');if(!fi.files[0])return;
 const fd=new FormData();fd.append('file',fi.files[0]);
 await fetch('/api/upload',{method:'POST',body:fd});load();
}

async function backup(){
 const r=await fetch('/api/backup');const d=await r.json();
 const blob=new Blob([JSON.stringify(d,null,2)],{type:'application/json'});
 const url=URL.createObjectURL(blob);
 const a=document.createElement('a');a.href=url;a.download='pandadeck_backup.json';a.click();
}

async function restore(){
 const fi=document.getElementById('restoreInput');
 if(!fi.files[0])return;
 if(fi.files[0].size>524288){alert('File too large (max 512KB).');return;}
 if(!confirm(L10N.confirm_restore))return;
 const reader=new FileReader();
 reader.onload=async(e)=>{
  await fetch('/api/restore',{method:'POST',body:e.target.result});
  alert(L10N.restore_ok);location.reload();
 };
 reader.readAsText(fi.files[0]);
}

async function del(name){
 if(!confirm(L10N.delete_file_confirm+name+'?'))return;
 const fd=new FormData();fd.append('filename',name);
 await fetch('/api/delete',{method:'POST',body:fd});load();
}

async function updateFirmware(){
 const file=document.getElementById('otaInput').files[0];
 if(!file){alert(L10N.please_select_bin);return;}
 const minSize=102400,maxSize=3145728;
 if(file.size<minSize||file.size>maxSize){
  alert(L10N.invalid_file_size+(file.size/(1024*1024)).toFixed(2)+'MB. '+L10N.must_be_between_100kb_3mb);
  return;
 }
 if(!confirm(L10N.update_firmware_confirm))return;
 const fd=new FormData();fd.append('update',file,file.name);
 const xhr=new XMLHttpRequest();
 xhr.open('POST','/api/update',true);
 const pc=document.getElementById('otaProgressContainer');
 const pb=document.getElementById('otaProgressBar');
 const ps=document.getElementById('otaProgressStatus');
 pc.style.display='block';
 ps.innerHTML=L10N.uploading_firmware+(file.size/(1024*1024)).toFixed(2)+'MB...';
 let lpu=0;
 xhr.upload.onprogress=(e)=>{
  if(e.lengthComputable){
   let p=(e.loaded/e.total)*100;if(p>99)p=99;
   const now=Date.now();
   if(now-lpu>250){
    pb.style.width=p+'%';
    ps.innerHTML=L10N.uploading+': '+p.toFixed(1)+'%';
    lpu=now;
   }
  }
 };
 xhr.onload=()=>{
  ps.style.fontSize='14px';
  if(xhr.status===200){
   pb.style.width='100%';ps.style.color='green';
   ps.innerHTML=L10N.update_successful;
   setTimeout(()=>{location.reload()},5000);
  }else{
   pb.style.width='0%';ps.style.color='red';
   ps.innerHTML=L10N.error+': '+(xhr.responseText||('HTTP '+xhr.status));
   setTimeout(()=>{pc.style.display='none'},5000);
  }
 };
 xhr.onerror=()=>{
  pb.style.width='0%';ps.style.color='red';
  ps.innerHTML=L10N.connection_failed;
  setTimeout(()=>{pc.style.display='none'},5000);
 };
 xhr.ontimeout=()=>{
  pb.style.width='0%';ps.style.color='red';
  ps.innerHTML=L10N.timeout_msg;
  setTimeout(()=>{pc.style.display='none'},8000);
 };
 xhr.timeout=300000;
 xhr.send(fd);
}

document.addEventListener('DOMContentLoaded',async()=>{
 const r=await fetch('/api/config');const d=await r.json();
 const container=document.getElementById('buttonContainer');
 container.innerHTML='';
 for(let i=0;i<20;i++){
  const div=document.createElement('div');
  div.className='card p-2 text-center btn-card';
  div.id='card'+i;
  div.innerHTML=`
   <b class='mb-2'>Button ${i+1}</b>
   <input type='text' name='b${i}l' class='form-control form-control-sm mb-1' placeholder='Name' maxlength='15'>
   <input type='text' name='b${i}v' id='val${i}' class='form-control form-control-sm mb-1 text-uppercase' placeholder='Command' maxlength='255'>
   <select name='b${i}t' id='type${i}' class='form-select form-select-sm mb-1' onchange='toggleBuilder(${i})'>
    <option value='0'>App (Win+R / Cmd+Space)</option>
    <option value='1'>Media Key</option>
    <option value='2'>Basic Combo (Ctrl/Cmd + Key)</option>
    <option value='3'>Advanced Combo</option>
   </select>
   <div id='basicHint${i}' class='small text-secondary mb-1 d-none' style='font-size:10px'>Basic combination uses Ctrl (Win) or Cmd (Mac) plus one key.</div>
   <div id='builder${i}' class='combo-builder d-none'>
    <div class='d-flex flex-wrap justify-content-center gap-1 mb-1'>
     <input type='checkbox' class='btn-check' id='c${i}' onchange='updC(${i})'><label class='btn btn-outline-info btn-xs py-0 px-1' style='font-size:10px' for='c${i}'>CTRL</label>
     <input type='checkbox' class='btn-check' id='s${i}' onchange='updC(${i})'><label class='btn btn-outline-info btn-xs py-0 px-1' style='font-size:10px' for='s${i}'>SHFT</label>
     <input type='checkbox' class='btn-check' id='a${i}' onchange='updC(${i})'><label class='btn btn-outline-info btn-xs py-0 px-1' style='font-size:10px' for='a${i}'>ALT</label>
     <input type='checkbox' class='btn-check' id='m${i}' onchange='updC(${i})'><label class='btn btn-outline-info btn-xs py-0 px-1' style='font-size:10px' for='m${i}'>META</label>
    </div>
    <select id='key${i}' class='form-select form-select-sm' style='font-size:11px' onchange='updC(${i})'></select>
   </div>
   <div class='d-flex gap-1 align-items-center mb-1 mt-1'>
    <input type='color' name='b${i}c' class='form-control form-control-color flex-grow-1' style='height:30px' title='Background Color'>
    <select name='b${i}icon' class='form-select form-select-sm icon-select' title='Icon'><option value='None'>None</option></select>
   </div>
   <select name='b${i}i' class='form-select form-select-sm asset-select' title='Custom Image'></select>
  `;
  container.appendChild(div);
 }

  const hiddenInputs=`
   <input type='hidden' id='osInput' name='os'>
   <input type='hidden' id='rowsInput' name='rows'>
   <input type='hidden' id='colsInput' name='cols'>
  `;
  container.insertAdjacentHTML('beforebegin',hiddenInputs);

 load();
});
</script>
</body></html>
)rawliteral";
