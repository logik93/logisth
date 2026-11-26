import {
  AdditiveBlending, BufferAttribute, BufferGeometry, Line, LineBasicMaterial,
  OrthographicCamera, Scene, WebGLRenderer,
} from "three";

export function createWaveScene(canvas: HTMLCanvasElement) {
  let renderer: WebGLRenderer;
  try {
    renderer = new WebGLRenderer({ canvas, alpha: true, antialias: true, powerPreference: "low-power" });
  } catch {
    return null;
  }

  const scene = new Scene();
  const camera = new OrthographicCamera(-1, 1, 1, -1, 0.1, 20);
  camera.position.z = 5;
  const samples = 144;
  const traces = Array.from({ length: 12 }, (_, index) => {
    const points = new BufferAttribute(new Float32Array(samples * 3), 3);
    const geometry = new BufferGeometry().setAttribute("position", points);
    const material = new LineBasicMaterial({
      color: index < 8 ? 0x9b79fa : 0x86e9f5,
      opacity: 0.13 + index * 0.008,
      transparent: true,
      blending: AdditiveBlending,
      depthWrite: false,
    });
    const line = new Line(geometry, material);
    line.frustumCulled = false;
    scene.add(line);
    return { points, geometry, material };
  });

  let width = 1;
  function resize() {
    const { clientWidth, clientHeight } = canvas;
    if (!clientWidth || !clientHeight) return;
    width = clientWidth / clientHeight;
    camera.left = -width;
    camera.right = width;
    camera.updateProjectionMatrix();
    renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 1.5));
    renderer.setSize(clientWidth, clientHeight, false);
  }
  const observer = new ResizeObserver(resize);
  observer.observe(canvas);
  resize();

  return {
    render(time: number, x: number, y: number) {
      traces.forEach(({ points }, index) => {
        for (let step = 0; step < samples; step++) {
          const u = step / (samples - 1);
          const envelope = Math.sin(u * Math.PI) ** 1.25;
          const ripple = Math.sin(u * 14 - time * 0.72 + index * 0.16);
          points.setXYZ(
            step,
            step / (samples - 1) * 2 * width - width,
            -0.62 + u * 0.55 + envelope * (ripple * 0.19 + (index - 5.5) * 0.044) + y * 0.07,
            envelope * Math.cos(u * 8 + x) * 0.25,
          );
        }
        points.needsUpdate = true;
      });
      renderer.render(scene, camera);
    },
    dispose() {
      observer.disconnect();
      for (const { geometry, material } of traces) {
        geometry.dispose();
        material.dispose();
      }
      renderer.dispose();
      renderer.forceContextLoss();
    },
  };
}
