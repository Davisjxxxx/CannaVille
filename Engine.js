/**
 * CannaVille Pro - Advanced 3D Engine Core
 * Cutting-edge WebGL 2.0 rendering engine with hyper-realistic capabilities
 */

import * as THREE from 'three';
import { GLTFLoader } from 'three/examples/jsm/loaders/GLTFLoader.js';
import { DRACOLoader } from 'three/examples/jsm/loaders/DRACOLoader.js';
import { KTX2Loader } from 'three/examples/jsm/loaders/KTX2Loader.js';
import { RGBELoader } from 'three/examples/jsm/loaders/RGBELoader.js';
import { EffectComposer } from 'three/examples/jsm/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/examples/jsm/postprocessing/RenderPass.js';
import { UnrealBloomPass } from 'three/examples/jsm/postprocessing/UnrealBloomPass.js';
import { SSAOPass } from 'three/examples/jsm/postprocessing/SSAOPass.js';
import { SMAAPass } from 'three/examples/jsm/postprocessing/SMAAPass.js';
import { OutputPass } from 'three/examples/jsm/postprocessing/OutputPass.js';
import Stats from 'stats.js';
import * as CANNON from 'cannon-es';
import { gsap } from 'gsap';

export class CannaVilleEngine {
    constructor() {
        this.container = null;
        this.renderer = null;
        this.scene = null;
        this.camera = null;
        this.composer = null;
        this.world = null;
        this.clock = new THREE.Clock();
        this.stats = null;
        
        // Asset management
        this.loadingManager = new THREE.LoadingManager();
        this.gltfLoader = null;
        this.textureLoader = new THREE.TextureLoader(this.loadingManager);
        this.ktx2Loader = null;
        this.rgbeLoader = new RGBELoader(this.loadingManager);
        
        // Performance monitoring
        this.frameCount = 0;
        this.lastFPSUpdate = 0;
        this.currentFPS = 60;
        this.targetFPS = 60;
        this.adaptiveQuality = true;
        
        // Device capabilities
        this.deviceInfo = this.detectDeviceCapabilities();
        this.qualitySettings = this.determineQualitySettings();
        
        // Event handlers
        this.onProgress = null;
        this.onLoad = null;
        this.onError = null;
        
        // Initialize loading manager callbacks
        this.setupLoadingManager();
    }
    
    /**
     * Detect device capabilities for adaptive quality
     */
    detectDeviceCapabilities() {
        const canvas = document.createElement('canvas');
        const gl = canvas.getContext('webgl2') || canvas.getContext('webgl');
        
        if (!gl) {
            throw new Error('WebGL not supported');
        }
        
        const info = {
            isMobile: /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent),
            isTablet: /iPad|Android/i.test(navigator.userAgent) && window.innerWidth > 768,
            supportsWebGL2: !!canvas.getContext('webgl2'),
            maxTextureSize: gl.getParameter(gl.MAX_TEXTURE_SIZE),
            maxRenderbufferSize: gl.getParameter(gl.MAX_RENDERBUFFER_SIZE),
            maxVertexAttribs: gl.getParameter(gl.MAX_VERTEX_ATTRIBS),
            maxVaryingVectors: gl.getParameter(gl.MAX_VARYING_VECTORS),
            maxFragmentUniforms: gl.getParameter(gl.MAX_FRAGMENT_UNIFORM_VECTORS),
            maxVertexUniforms: gl.getParameter(gl.MAX_VERTEX_UNIFORM_VECTORS),
            extensions: {
                anisotropic: gl.getExtension('EXT_texture_filter_anisotropic'),
                floatTextures: gl.getExtension('OES_texture_float'),
                depthTexture: gl.getExtension('WEBGL_depth_texture'),
                drawBuffers: gl.getExtension('WEBGL_draw_buffers')
            }
        };
        
        // Estimate GPU performance tier
        const renderer = gl.getParameter(gl.RENDERER);
        info.gpuTier = this.estimateGPUTier(renderer);
        
        // Memory estimation
        const memInfo = gl.getExtension('WEBGL_debug_renderer_info');
        if (memInfo) {
            info.gpu = gl.getParameter(memInfo.UNMASKED_RENDERER_WEBGL);
        }
        
        canvas.remove();
        return info;
    }
    
    /**
     * Estimate GPU performance tier
     */
    estimateGPUTier(renderer) {
        const lowEnd = /Mali|PowerVR|Adreno [0-9][0-9][0-9]|Intel/i;
        const midRange = /Adreno [4-6][0-9][0-9]|GeForce GTX [0-9][0-9][0-9]|Radeon/i;
        const highEnd = /GeForce RTX|GeForce GTX 1[0-9][0-9][0-9]|Radeon RX/i;
        
        if (highEnd.test(renderer)) return 'high';
        if (midRange.test(renderer)) return 'medium';
        if (lowEnd.test(renderer)) return 'low';
        return 'medium'; // Default fallback
    }
    
    /**
     * Determine quality settings based on device capabilities
     */
    determineQualitySettings() {
        const { isMobile, gpuTier, supportsWebGL2 } = this.deviceInfo;
        
        const settings = {
            shadowMapSize: 1024,
            maxLights: 4,
            enableSSAO: false,
            enableBloom: true,
            enableAntialiasing: true,
            pixelRatio: Math.min(window.devicePixelRatio, 2),
            lodBias: 0,
            textureQuality: 1.0,
            particleDensity: 1.0,
            enablePostProcessing: true
        };
        
        if (isMobile) {
            settings.shadowMapSize = 512;
            settings.maxLights = 2;
            settings.enableSSAO = false;
            settings.pixelRatio = Math.min(window.devicePixelRatio, 1.5);
            settings.lodBias = 1;
            settings.textureQuality = 0.75;
            settings.particleDensity = 0.5;
        }
        
        if (gpuTier === 'high' && supportsWebGL2) {
            settings.shadowMapSize = 2048;
            settings.maxLights = 8;
            settings.enableSSAO = true;
            settings.textureQuality = 1.0;
            settings.particleDensity = 1.5;
        } else if (gpuTier === 'low') {
            settings.shadowMapSize = 256;
            settings.maxLights = 1;
            settings.enableBloom = false;
            settings.enableAntialiasing = false;
            settings.enablePostProcessing = false;
            settings.textureQuality = 0.5;
            settings.particleDensity = 0.25;
        }
        
        return settings;
    }
    
    /**
     * Setup loading manager with progress tracking
     */
    setupLoadingManager() {
        this.loadingManager.onProgress = (url, itemsLoaded, itemsTotal) => {
            const progress = (itemsLoaded / itemsTotal) * 100;
            if (this.onProgress) {
                this.onProgress(progress, `Loading ${url.split('/').pop()}`);
            }
        };
        
        this.loadingManager.onLoad = () => {
            if (this.onLoad) {
                this.onLoad();
            }
        };
        
        this.loadingManager.onError = (url) => {
            console.error('Failed to load:', url);
            if (this.onError) {
                this.onError(url);
            }
        };
    }
    
    /**
     * Initialize the 3D engine
     */
    async init(container) {
        this.container = container;
        
        // Create scene
        this.scene = new THREE.Scene();
        this.scene.fog = new THREE.Fog(0x87CEEB, 50, 200);
        
        // Setup camera
        this.setupCamera();
        
        // Setup renderer
        this.setupRenderer();
        
        // Setup physics world
        this.setupPhysics();
        
        // Setup loaders
        this.setupLoaders();
        
        // Setup lighting
        this.setupLighting();
        
        // Setup post-processing
        this.setupPostProcessing();
        
        // Setup performance monitoring
        this.setupPerformanceMonitoring();
        
        // Setup event listeners
        this.setupEventListeners();
        
        // Start render loop
        this.animate();
        
        console.log('CannaVille Pro Engine initialized with quality settings:', this.qualitySettings);
    }
    
    /**
     * Setup camera with adaptive FOV
     */
    setupCamera() {
        const aspect = window.innerWidth / window.innerHeight;
        const fov = this.deviceInfo.isMobile ? 75 : 60; // Wider FOV for mobile
        
        this.camera = new THREE.PerspectiveCamera(fov, aspect, 0.1, 1000);
        this.camera.position.set(0, 5, 10);
        this.camera.lookAt(0, 0, 0);
    }
    
    /**
     * Setup WebGL renderer with advanced features
     */
    setupRenderer() {
        this.renderer = new THREE.WebGLRenderer({
            antialias: this.qualitySettings.enableAntialiasing,
            alpha: false,
            powerPreference: 'high-performance',
            stencil: false,
            depth: true,
            logarithmicDepthBuffer: false
        });
        
        this.renderer.setSize(window.innerWidth, window.innerHeight);
        this.renderer.setPixelRatio(this.qualitySettings.pixelRatio);
        
        // Enable advanced features
        this.renderer.shadowMap.enabled = true;
        this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
        this.renderer.shadowMap.autoUpdate = true;
        
        this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
        this.renderer.toneMappingExposure = 1.0;
        this.renderer.outputColorSpace = THREE.SRGBColorSpace;
        
        // Enable WebGL 2.0 features if available
        if (this.deviceInfo.supportsWebGL2) {
            this.renderer.capabilities.isWebGL2 = true;
        }
        
        this.container.appendChild(this.renderer.domElement);
    }
    
    /**
     * Setup physics world
     */
    setupPhysics() {
        this.world = new CANNON.World({
            gravity: new CANNON.Vec3(0, -9.82, 0),
            broadphase: new CANNON.NaiveBroadphase(),
            solver: new CANNON.GSSolver()
        });
        
        this.world.defaultContactMaterial.friction = 0.4;
        this.world.defaultContactMaterial.restitution = 0.3;
        
        // Ground plane
        const groundShape = new CANNON.Plane();
        const groundBody = new CANNON.Body({ mass: 0 });
        groundBody.addShape(groundShape);
        groundBody.quaternion.setFromAxisAngle(new CANNON.Vec3(1, 0, 0), -Math.PI / 2);
        this.world.addBody(groundBody);
    }
    
    /**
     * Setup asset loaders with optimization
     */
    setupLoaders() {
        // DRACO loader for compressed geometry
        const dracoLoader = new DRACOLoader();
        dracoLoader.setDecoderPath('/libs/draco/');
        dracoLoader.setDecoderConfig({ type: 'js' });
        
        // GLTF loader with DRACO support
        this.gltfLoader = new GLTFLoader(this.loadingManager);
        this.gltfLoader.setDRACOLoader(dracoLoader);
        
        // KTX2 loader for compressed textures
        this.ktx2Loader = new KTX2Loader(this.loadingManager);
        this.ktx2Loader.setTranscoderPath('/libs/basis/');
        this.ktx2Loader.detectSupport(this.renderer);
    }
    
    /**
     * Setup realistic lighting system
     */
    setupLighting() {
        // Environment lighting
        this.rgbeLoader.load('/assets/textures/environment.hdr', (texture) => {
            texture.mapping = THREE.EquirectangularReflectionMapping;
            this.scene.environment = texture;
            this.scene.background = texture;
        });
        
        // Directional light (sun)
        const directionalLight = new THREE.DirectionalLight(0xffffff, 1.0);
        directionalLight.position.set(10, 20, 5);
        directionalLight.castShadow = true;
        directionalLight.shadow.mapSize.setScalar(this.qualitySettings.shadowMapSize);
        directionalLight.shadow.camera.near = 0.1;
        directionalLight.shadow.camera.far = 50;
        directionalLight.shadow.camera.left = -20;
        directionalLight.shadow.camera.right = 20;
        directionalLight.shadow.camera.top = 20;
        directionalLight.shadow.camera.bottom = -20;
        this.scene.add(directionalLight);
        
        // Ambient light
        const ambientLight = new THREE.AmbientLight(0x404040, 0.3);
        this.scene.add(ambientLight);
        
        // LED grow lights (for indoor scenes)
        this.createGrowLights();
    }
    
    /**
     * Create LED grow lights for indoor cultivation
     */
    createGrowLights() {
        const growLightGeometry = new THREE.PlaneGeometry(2, 1);
        const growLightMaterial = new THREE.MeshBasicMaterial({
            color: 0xff69b4,
            emissive: 0xff69b4,
            emissiveIntensity: 0.5
        });
        
        for (let i = 0; i < 2; i++) {
            const growLight = new THREE.Mesh(growLightGeometry, growLightMaterial);
            growLight.position.set(i * 4 - 2, 8, 0);
            growLight.rotation.x = -Math.PI / 2;
            this.scene.add(growLight);
            
            // Point light for illumination
            const pointLight = new THREE.PointLight(0xff69b4, 2, 10);
            pointLight.position.copy(growLight.position);
            pointLight.castShadow = true;
            pointLight.shadow.mapSize.setScalar(this.qualitySettings.shadowMapSize / 2);
            this.scene.add(pointLight);
        }
    }
    
    /**
     * Setup post-processing pipeline
     */
    setupPostProcessing() {
        if (!this.qualitySettings.enablePostProcessing) {
            return;
        }
        
        this.composer = new EffectComposer(this.renderer);
        
        // Render pass
        const renderPass = new RenderPass(this.scene, this.camera);
        this.composer.addPass(renderPass);
        
        // SSAO pass
        if (this.qualitySettings.enableSSAO) {
            const ssaoPass = new SSAOPass(this.scene, this.camera, window.innerWidth, window.innerHeight);
            ssaoPass.kernelRadius = 16;
            ssaoPass.minDistance = 0.005;
            ssaoPass.maxDistance = 0.1;
            this.composer.addPass(ssaoPass);
        }
        
        // Bloom pass
        if (this.qualitySettings.enableBloom) {
            const bloomPass = new UnrealBloomPass(
                new THREE.Vector2(window.innerWidth, window.innerHeight),
                0.5, 0.4, 0.85
            );
            this.composer.addPass(bloomPass);
        }
        
        // Anti-aliasing
        if (this.qualitySettings.enableAntialiasing) {
            const smaaPass = new SMAAPass(window.innerWidth, window.innerHeight);
            this.composer.addPass(smaaPass);
        }
        
        // Output pass
        const outputPass = new OutputPass();
        this.composer.addPass(outputPass);
    }
    
    /**
     * Setup performance monitoring
     */
    setupPerformanceMonitoring() {
        this.stats = new Stats();
        this.stats.showPanel(0); // FPS panel
        this.stats.dom.style.position = 'absolute';
        this.stats.dom.style.top = '10px';
        this.stats.dom.style.left = '10px';
        this.stats.dom.style.zIndex = '1000';
        
        if (!this.deviceInfo.isMobile) {
            document.body.appendChild(this.stats.dom);
        }
    }
    
    /**
     * Setup event listeners
     */
    setupEventListeners() {
        window.addEventListener('resize', this.onWindowResize.bind(this));
        window.addEventListener('orientationchange', this.onOrientationChange.bind(this));
        
        // Visibility API for performance optimization
        document.addEventListener('visibilitychange', () => {
            if (document.hidden) {
                this.targetFPS = 15; // Reduce FPS when tab is hidden
            } else {
                this.targetFPS = this.deviceInfo.isMobile ? 30 : 60;
            }
        });
    }
    
    /**
     * Handle window resize
     */
    onWindowResize() {
        const width = window.innerWidth;
        const height = window.innerHeight;
        
        this.camera.aspect = width / height;
        this.camera.updateProjectionMatrix();
        
        this.renderer.setSize(width, height);
        
        if (this.composer) {
            this.composer.setSize(width, height);
        }
    }
    
    /**
     * Handle orientation change
     */
    onOrientationChange() {
        setTimeout(() => {
            this.onWindowResize();
        }, 100);
    }
    
    /**
     * Adaptive quality management
     */
    updateAdaptiveQuality() {
        if (!this.adaptiveQuality) return;
        
        const now = performance.now();
        this.frameCount++;
        
        if (now - this.lastFPSUpdate > 1000) {
            this.currentFPS = this.frameCount;
            this.frameCount = 0;
            this.lastFPSUpdate = now;
            
            // Adjust quality based on performance
            if (this.currentFPS < this.targetFPS * 0.8) {
                this.reduceQuality();
            } else if (this.currentFPS > this.targetFPS * 0.95) {
                this.increaseQuality();
            }
        }
    }
    
    /**
     * Reduce quality for better performance
     */
    reduceQuality() {
        if (this.qualitySettings.pixelRatio > 0.5) {
            this.qualitySettings.pixelRatio *= 0.9;
            this.renderer.setPixelRatio(this.qualitySettings.pixelRatio);
        }
    }
    
    /**
     * Increase quality when performance allows
     */
    increaseQuality() {
        const maxPixelRatio = this.deviceInfo.isMobile ? 1.5 : 2;
        if (this.qualitySettings.pixelRatio < maxPixelRatio) {
            this.qualitySettings.pixelRatio *= 1.05;
            this.renderer.setPixelRatio(this.qualitySettings.pixelRatio);
        }
    }
    
    /**
     * Main animation loop
     */
    animate() {
        requestAnimationFrame(this.animate.bind(this));
        
        if (this.stats) {
            this.stats.begin();
        }
        
        const deltaTime = this.clock.getDelta();
        
        // Update physics
        if (this.world) {
            this.world.step(deltaTime);
        }
        
        // Update adaptive quality
        this.updateAdaptiveQuality();
        
        // Render
        if (this.composer) {
            this.composer.render();
        } else {
            this.renderer.render(this.scene, this.camera);
        }
        
        if (this.stats) {
            this.stats.end();
        }
    }
    
    /**
     * Load GLTF model with optimization
     */
    async loadModel(url, options = {}) {
        return new Promise((resolve, reject) => {
            this.gltfLoader.load(
                url,
                (gltf) => {
                    const model = gltf.scene;
                    
                    // Optimize model
                    this.optimizeModel(model, options);
                    
                    resolve(model);
                },
                undefined,
                reject
            );
        });
    }
    
    /**
     * Optimize loaded model
     */
    optimizeModel(model, options) {
        model.traverse((child) => {
            if (child.isMesh) {
                // Enable shadows
                child.castShadow = true;
                child.receiveShadow = true;
                
                // Optimize materials
                if (child.material) {
                    child.material.needsUpdate = true;
                    
                    // Enable anisotropic filtering if supported
                    if (this.deviceInfo.extensions.anisotropic && child.material.map) {
                        child.material.map.anisotropy = this.renderer.capabilities.getMaxAnisotropy();
                    }
                }
                
                // Frustum culling
                child.frustumCulled = true;
            }
        });
        
        // Apply LOD if specified
        if (options.lod) {
            this.applyLOD(model, options.lod);
        }
    }
    
    /**
     * Apply Level of Detail (LOD) to model
     */
    applyLOD(model, lodLevels) {
        const lod = new THREE.LOD();
        
        lodLevels.forEach((level, index) => {
            const lodModel = model.clone();
            
            // Simplify geometry for higher LOD levels
            if (index > 0) {
                this.simplifyModel(lodModel, level.simplification);
            }
            
            lod.addLevel(lodModel, level.distance);
        });
        
        return lod;
    }
    
    /**
     * Simplify model for LOD
     */
    simplifyModel(model, simplificationFactor) {
        model.traverse((child) => {
            if (child.isMesh && child.geometry) {
                // Simple vertex reduction (in production, use proper decimation)
                const geometry = child.geometry;
                const positions = geometry.attributes.position.array;
                const newPositions = new Float32Array(positions.length * simplificationFactor);
                
                for (let i = 0; i < newPositions.length; i++) {
                    newPositions[i] = positions[i];
                }
                
                geometry.setAttribute('position', new THREE.BufferAttribute(newPositions, 3));
            }
        });
    }
    
    /**
     * Dispose of resources
     */
    dispose() {
        if (this.renderer) {
            this.renderer.dispose();
        }
        
        if (this.composer) {
            this.composer.dispose();
        }
        
        if (this.stats && this.stats.dom.parentNode) {
            this.stats.dom.parentNode.removeChild(this.stats.dom);
        }
        
        // Dispose of scene objects
        this.scene.traverse((object) => {
            if (object.geometry) {
                object.geometry.dispose();
            }
            
            if (object.material) {
                if (Array.isArray(object.material)) {
                    object.material.forEach(material => material.dispose());
                } else {
                    object.material.dispose();
                }
            }
        });
    }
}

