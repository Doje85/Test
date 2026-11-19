#pragma once
#include <pgmspace.h>

static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="de"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>NeoPixel-Ringe</title>
<style>
:root{--bg:#111418;--card:#1c2128;--fg:#e6e8eb;--mute:#8b949e;--acc:#3b82f6}
*{box-sizing:border-box}
body{margin:0;font-family:system-ui,sans-serif;background:var(--bg);color:var(--fg)}
header{display:flex;justify-content:space-between;align-items:center;padding:16px}
h1{font-size:1.3rem;margin:0}
main{display:grid;gap:12px;padding:0 16px 24px;grid-template-columns:repeat(auto-fill,minmax(280px,1fr))}
.card{background:var(--card);border-radius:12px;padding:14px}
.card h2{font-size:1rem;margin:0 0 4px;display:flex;justify-content:space-between;align-items:center}
.card small{color:var(--mute)}
.all{border:1px solid var(--acc)}
label{display:flex;justify-content:space-between;align-items:center;gap:10px;margin-top:10px;font-size:.9rem}
input[type=range]{flex:1}
select{flex:1;background:#0d1117;color:var(--fg);border:1px solid #30363d;border-radius:6px;padding:6px}
input[type=color]{width:56px;height:32px;border:none;background:none;padding:0}
button{background:var(--acc);color:#fff;border:0;border-radius:8px;padding:8px 14px;font-size:.9rem;cursor:pointer}
.sw{position:relative;width:44px;height:24px;flex:none}
.sw input{opacity:0;width:0;height:0}
.sw span{position:absolute;inset:0;background:#30363d;border-radius:24px;transition:.2s}
.sw span:before{content:"";position:absolute;width:18px;height:18px;left:3px;top:3px;background:#fff;border-radius:50%;transition:.2s}
.sw input:checked+span{background:var(--acc)}
.sw input:checked+span:before{transform:translateX(20px)}
#msg{color:var(--mute);font-size:.85rem;margin-right:10px}
</style></head><body>
<header><h1>NeoPixel-Ringe</h1><div><span id="msg"></span><button id="save">Speichern</button></div></header>
<main id="rings"></main>
<script>
let S=null;const $=s=>document.querySelector(s);
const tmr={};
function send(url,params,key){
  clearTimeout(tmr[key]);
  tmr[key]=setTimeout(()=>{
    fetch(url+'?'+new URLSearchParams(params),{method:'POST'}).then(r=>r.json()).then(j=>{S=j;if(key.startsWith('all'))render(true);});
  },80);
}
function card(r,isAll){
  const id=isAll?'all':r.id;
  const opts=S.effects.map((n,i)=>`<option value="${i}"${i==r.effect?' selected':''}>${n}</option>`).join('');
  const el=document.createElement('div');el.className='card'+(isAll?' all':'');
  el.innerHTML=`<h2>${isAll?'Alle Ringe':'Ring '+(r.id+1)}
   <label class="sw"><input type="checkbox" data-k="on"${r.on?' checked':''}><span></span></label></h2>
   <small>${isAll?'Änderungen gelten für alle Ringe':'GPIO '+r.pin+' · '+r.leds+' LEDs'}</small>
   <label>Farbe <input type="color" data-k="color" value="${r.color}"></label>
   <label>Effekt <select data-k="effect">${opts}</select></label>
   <label>Helligkeit <input type="range" min="0" max="${S.maxBrightness}" data-k="brightness" value="${r.brightness}"></label>
   <label>Tempo <input type="range" min="1" max="100" data-k="speed" value="${r.speed}"></label>
   <label>Rückwärts <span class="sw"><input type="checkbox" data-k="reverse"${r.reverse?' checked':''}><span></span></span></label>`;
  el.querySelectorAll('[data-k]').forEach(inp=>{
    const k=inp.dataset.k;
    inp.addEventListener(inp.type=='range'||inp.type=='color'?'input':'change',()=>{
      const v=inp.type=='checkbox'?(inp.checked?1:0):inp.value;
      const p={[k]:v};
      if(isAll)send('/api/all',p,'all'+k);else send('/api/ring',{id:r.id,...p},id+k);
    });
  });
  return el;
}
function render(keepAll){
  const m=$('#rings');
  if(keepAll){while(m.children.length>1)m.removeChild(m.lastChild);}
  else{m.innerHTML='';m.appendChild(card(S.rings[0],true));}
  S.rings.forEach(r=>m.appendChild(card(r,false)));
}
$('#save').onclick=()=>fetch('/api/save',{method:'POST'}).then(()=>{$('#msg').textContent='Gespeichert ✓';setTimeout(()=>$('#msg').textContent='',2000);});
fetch('/api/state').then(r=>r.json()).then(j=>{S=j;render();});
</script></body></html>)HTML";
