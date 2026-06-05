import * as THREE from "three";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";

const wanted = new URLSearchParams(location.search).get("m") || "";
const canvas = document.querySelector("#c");

const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.setClearColor(0x000000, 0);
renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(35, 1, 0.01, 5000);

scene.add(new THREE.HemisphereLight(0xffffff, 0x7b7563, 2.2));
const key = new THREE.DirectionalLight(0xffffff, 1.8);
key.position.set(3, 6, 4);
scene.add(key);
const rim = new THREE.DirectionalLight(0xb7d8ff, 0.8);
rim.position.set(-4, 3, -5);
scene.add(rim);

// ── built-in orbit (no external controls dep) ──────────────────────────────
let yaw = 0.7;
let pitch = 0.35;
let dist = 3;
let distMin = 0.5;
let distMax = 30;
let autoRotate = true;

const pointers = new Map();
let lastPinch = 0;

function place() {
  const cp = Math.cos(pitch);
  camera.position.set(
    dist * cp * Math.sin(yaw),
    dist * Math.sin(pitch),
    dist * cp * Math.cos(yaw)
  );
  camera.lookAt(0, 0, 0);
}

canvas.addEventListener("pointerdown", (e) => {
  canvas.setPointerCapture(e.pointerId);
  pointers.set(e.pointerId, { x: e.clientX, y: e.clientY });
  autoRotate = false;
});
canvas.addEventListener("pointermove", (e) => {
  if (!pointers.has(e.pointerId)) return;
  const prev = pointers.get(e.pointerId);
  const dx = e.clientX - prev.x;
  const dy = e.clientY - prev.y;
  pointers.set(e.pointerId, { x: e.clientX, y: e.clientY });
  if (pointers.size === 1) {
    yaw -= dx * 0.01;
    pitch += dy * 0.01;
    pitch = Math.max(-1.4, Math.min(1.4, pitch));
  } else if (pointers.size === 2) {
    const p = [...pointers.values()];
    const d = Math.hypot(p[0].x - p[1].x, p[0].y - p[1].y);
    if (lastPinch) {
      dist *= lastPinch / d;
      dist = Math.max(distMin, Math.min(distMax, dist));
    }
    lastPinch = d;
  }
});
function endPointer(e) {
  pointers.delete(e.pointerId);
  if (pointers.size < 2) lastPinch = 0;
}
canvas.addEventListener("pointerup", endPointer);
canvas.addEventListener("pointercancel", endPointer);
canvas.addEventListener(
  "wheel",
  (e) => {
    e.preventDefault();
    dist *= 1 + e.deltaY * 0.0012;
    dist = Math.max(distMin, Math.min(distMax, dist));
    autoRotate = false;
  },
  { passive: false }
);

function resize() {
  const rect = canvas.getBoundingClientRect();
  const w = Math.max(1, rect.width);
  const h = Math.max(1, rect.height);
  const ratio = renderer.getPixelRatio();
  if (canvas.width !== Math.floor(w * ratio) || canvas.height !== Math.floor(h * ratio)) {
    renderer.setSize(w, h, false);
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
  }
}

function frameObject(object) {
  const box = new THREE.Box3().setFromObject(object);
  const center = box.getCenter(new THREE.Vector3());
  const size = box.getSize(new THREE.Vector3());
  object.position.sub(center); // center at origin
  const radius = Math.max(size.x, size.y, size.z, 1);
  dist = radius * 2.2;
  distMin = radius * 0.5;
  distMax = radius * 10;
  camera.near = Math.max(radius / 500, 0.01);
  camera.far = radius * 100;
  camera.updateProjectionMatrix();
}

function loop() {
  resize();
  if (autoRotate) yaw += 0.0045;
  place();
  renderer.render(scene, camera);
  requestAnimationFrame(loop);
}

function niceName(item) {
  return (item.source_grn || item.name || "")
    .replace(/\\/g, "/")
    .replace(/^grn\//i, "")
    .replace(/\.grn$/i, "");
}

async function main() {
  const catalog = await fetch("./data/catalog.json").then((r) => r.json());
  const item =
    catalog.find((i) => i.name === wanted) ||
    catalog.find((i) => i.model === wanted);

  if (!item) {
    document.querySelector("#v-name").textContent = "Mesh not found";
    return;
  }

  const name = niceName(item);
  document.title = `${name} · GRN mesh`;
  document.querySelector("#v-name").textContent = name;
  document.querySelector("#v-tag").textContent = item.descp || "";
  document.querySelector("#v-meta").innerHTML =
    `<span>${item.texture_name || item.texture || ""}</span>` +
    `<span>${item.material || item.source_bmp || ""}</span>` +
    `<span>${item.verts} verts · ${item.tris} tris · ${item.dims}</span>`;

  const gltf = await new GLTFLoader().loadAsync(item.model);
  gltf.scene.traverse((node) => {
    if (node.isMesh) {
      node.frustumCulled = false;
      if (node.material) node.material.side = THREE.DoubleSide;
    }
  });
  scene.add(gltf.scene);
  frameObject(gltf.scene);
  loop();
}

main().catch((err) => {
  console.error(err);
  document.querySelector("#v-name").textContent = "Load failed";
});
