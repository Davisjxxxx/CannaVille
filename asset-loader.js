/**
 * CannaVille Pro - Enhanced Asset Loader with LOD System
 * Implements the exact specifications from the asset brief
 */

import { LOD } from 'three';
import { GLTFLoader } from 'three/examples/jsm/loaders/GLTFLoader.js';
import { DRACOLoader } from 'three/examples/jsm/loaders/DRACOLoader.js';
import { KTX2Loader } from 'three/examples/jsm/loaders/KTX2Loader.js';
import { TextureLoader } from 'three';

class CannaVilleAssetLoader {
    constructor(renderer) {
        this.renderer = renderer;
        this.loadingManager = new THREE.LoadingManager();
        this.cache = new Map();
        
        // Initialize DRACO loader
        this.dracoLoader = new DRACOLoader();
        this.dracoLoader.setDecoderPath('/libs/draco/');
        
        // Initialize KTX2 loader for compressed textures
        this.ktx2Loader = new KTX2Loader();
        this.ktx2Loader.setTranscoderPath('/libs/basis/');
        this.ktx2Loader.detectSupport(renderer);
        
        // Initialize GLTF loader with compression support
        this.gltfLoader = new GLTFLoader(this.loadingManager);
        this.gltfLoader.setDRACOLoader(this.dracoLoader);
        this.gltfLoader.setKTX2Loader(this.ktx2Loader);
        
        // Standard texture loader
        this.textureLoader = new TextureLoader(this.loadingManager);
        
        // LOD distances as specified in brief
        this.lodDistances = [0, 8, 18];
        
        // Asset manifest
        this.manifest = null;
        this.loadManifest();
        
        // Performance monitoring
        this.stats = {
            totalTriangles: 0,
            drawCalls: 0,
            loadedAssets: 0,
            memoryUsage: 0
        };
    }
    
    async loadManifest() {
        try {
            const response = await fetch('/assets/manifest.json');
            this.manifest = await response.json();
            console.log('Asset manifest loaded:', this.manifest);
        } catch (error) {
            console.error('Failed to load asset manifest:', error);
        }
    }
    
    /**
     * Create LOD object with multiple detail levels
     * @param {string} basePath - Base path without LOD suffix
     * @param {Array} distances - LOD switch distances [LOD0, LOD1, LOD2]
     * @returns {LOD} Three.js LOD object
     */
    createLOD(basePath, distances = this.lodDistances) {
        const lod = new LOD();
        const lodLevels = ['LOD0', 'LOD1', 'LOD2'];
        
        lodLevels.forEach((level, index) => {
            const path = `${basePath}_${level}.glb`;
            
            this.gltfLoader.load(
                path,
                (gltf) => {
                    const model = gltf.scene;
                    
                    // Apply optimizations based on LOD level
                    this.optimizeModel(model, index);
                    
                    // Add to LOD with specified distance
                    lod.addLevel(model, distances[index]);
                    
                    // Update stats
                    this.updateStats(model, level);
                    
                    console.log(`Loaded ${level}: ${path}`);
                },
                (progress) => {
                    console.log(`Loading ${level}: ${(progress.loaded / progress.total * 100)}%`);
                },
                (error) => {
                    console.error(`Failed to load ${level}: ${path}`, error);
                }
            );
        });
        
        return lod;
    }
    
    /**
     * Load cannabis plant with specific growth stage
     * @param {string} stage - Growth stage: seedling, vegetative, flowering, harvest
     * @param {number} variant - Plant variant (0 or 1)
     * @returns {LOD} Cannabis plant LOD object
     */
    loadCannabisPlant(stage = 'vegetative', variant = 0) {
        let plantPath;
        switch(stage) {
            case 'seedling':
                plantPath = '/assets/models/plants/small_cannabis_plant.glb';
                break;
            case 'vegetative':
                plantPath = '/assets/models/plants/cannabis_plant_small_1.glb';
                break;
            case 'flowering':
                plantPath = '/assets/models/plants/cannabis_plant.glb';
                break;
            case 'harvest':
                plantPath = '/assets/models/plants/cannabis_sativa_plant.glb';
                break;
            default:
                plantPath = '/assets/models/plants/cannabis_plant.glb';
        }

        const lod = new LOD();
        this.gltfLoader.load(
            plantPath,
            (gltf) => {
                const model = gltf.scene;
                lod.addLevel(model, 0); // Add at distance 0 for now, will implement proper LOD later
                console.log(`Loaded plant for stage ${stage}: ${plantPath}`);
            },
            (progress) => {
                console.log(`Loading plant: ${(progress.loaded / progress.total * 100)}%`);
            },
            (error) => {
                console.error(`Failed to load plant: ${plantPath}`, error);
            }
        );

        // Add plant-specific properties
        lod.userData = {
            type: 'cannabis_plant',
            stage: stage,
            variant: variant,
            health: 1.0,
            growthProgress: this.getGrowthProgress(stage)
        };

        return lod;
    }
    
    /**
     * Load DWC hydroponic system components
     * @returns {Object} Object containing all hydro components
     */
    loadDWCSystem() {
        const system = {
            buckets: [],
            reservoir: null,
            pvcNetwork: null,
            ledBoards: [],
            tent: null,
            tray: null
        };
        
        // Load 6 DWC buckets in specified positions
        const bucketPositions = this.calculateBucketPositions();
        bucketPositions.forEach((position, index) => {
            const bucket = this.createLOD('/assets/models/optimized/dwc_bucket');
            bucket.position.copy(position);
            bucket.userData = {
                type: 'dwc_bucket',
                id: index,
                waterLevel: 0.8,
                nutrients: { ec: 1.2, ph: 6.0 }
            };
            system.buckets.push(bucket);
        });
        
        // Load central reservoir and pump
        system.reservoir = this.createLOD('/assets/models/optimized/reservoir_pump');
        system.reservoir.position.set(0, 0, -0.2);
        system.reservoir.userData = {
            type: 'reservoir',
            capacity: 50, // liters
            currentLevel: 40
        };
        
        // Load PVC network
        system.pvcNetwork = this.createLOD('/assets/models/optimized/pvc_network');
        system.pvcNetwork.userData = {
            type: 'pvc_network',
            flowRate: 2.5 // liters per minute
        };
        
        // Load 2 LED quantum boards
        const ledPositions = [
            new THREE.Vector3(-0.3, 0, 1.8),
            new THREE.Vector3(0.3, 0, 1.8)
        ];
        ledPositions.forEach((position, index) => {
            const led = this.createLOD('/assets/models/optimized/led_quantum');
            led.position.copy(position);
            led.userData = {
                type: 'led_board',
                id: index,
                power: 240, // watts
                spectrum: 'full',
                intensity: 0.8
            };
            system.ledBoards.push(led);
        });
        
        // Load mylar tent
        system.tent = this.createLOD('/assets/models/optimized/mylar_tent');
        system.tent.userData = {
            type: 'tent',
            size: { width: 1.2, length: 1.2, height: 2.0 }
        };
        
        // Load aluminum tray
        system.tray = this.createLOD('/assets/models/optimized/aluminum_tray');
        system.tray.position.set(0, 0, -0.1);
        system.tray.userData = {
            type: 'tray',
            material: 'aluminum',
            drainage: true
        };
        
        return system;
    }
    
    /**
     * Load outdoor growing plot with 5x5 grid
     * @returns {Object} Outdoor plot components
     */
    loadOutdoorPlot() {
        const plot = {
            grassTiles: [],
            soilHoles: [],
            plants: []
        };
        
        // Load grass tile (can be instanced)
        const grassTile = this.createLOD('/assets/models/optimized/outdoor_tile');
        
        // Apply grass texture
        this.textureLoader.load('/assets/textures/grass_seamless.jpg', (texture) => {
            texture.wrapS = texture.wrapT = THREE.RepeatWrapping;
            texture.repeat.set(1, 1);
            
            grassTile.traverse((child) => {
                if (child.isMesh && child.material) {
                    child.material.map = texture;
                    child.material.needsUpdate = true;
                }
            });
        });
        
        plot.grassTiles.push(grassTile);
        
        // Create 5x5 grid of soil holes with plants
        for (let x = 0; x < 5; x++) {
            for (let y = 0; y < 5; y++) {
                const position = new THREE.Vector3(
                    -1.2 + (x * 0.6), // 0.6m spacing as specified
                    -1.2 + (y * 0.6),
                    0
                );
                
                // Add plant at this position
                const plant = this.loadCannabisPlant('seedling', Math.floor(Math.random() * 2));
                plant.position.copy(position);
                plant.userData.plotPosition = { x, y };
                plot.plants.push(plant);
            }
        }
        
        return plot;
    }
    
    /**
     * Load VALID avatar system
     * @param {string} ethnicity - Avatar ethnicity
     * @param {string} gender - Avatar gender (male/female)
     * @returns {LOD} Avatar LOD object
     */
    loadAvatar(ethnicity = 'caucasian', gender = 'male') {
        const basePath = `/assets/models/avatars/valid_${ethnicity}_${gender}`;
        const avatar = this.createLOD(basePath);
        
        avatar.userData = {
            type: 'avatar',
            ethnicity: ethnicity,
            gender: gender,
            animations: ['idle', 'walk', 'water', 'harvest'],
            expertise: this.getAvatarExpertise(ethnicity, gender)
        };
        
        return avatar;
    }
    
    /**
     * Calculate bucket positions for 6-bucket DWC system
     * @returns {Array} Array of Vector3 positions
     */
    calculateBucketPositions() {
        const positions = [];
        const spacing = 0.35; // meters between bucket centers
        
        // 2 rows of 3 buckets each
        for (let row = 0; row < 2; row++) {
            for (let col = 0; col < 3; col++) {
                positions.push(new THREE.Vector3(
                    -spacing + (col * spacing),
                    -0.175 + (row * 0.35),
                    0
                ));
            }
        }
        
        return positions;
    }
    
    /**
     * Get growth progress based on stage
     * @param {string} stage - Growth stage
     * @returns {number} Progress value 0-1
     */
    getGrowthProgress(stage) {
        const progressMap = {
            'seedling': 0.1,
            'vegetative': 0.4,
            'flowering': 0.8,
            'harvest': 1.0
        };
        return progressMap[stage] || 0.5;
    }
    
    /**
     * Get avatar expertise based on ethnicity and gender
     * @param {string} ethnicity - Avatar ethnicity
     * @param {string} gender - Avatar gender
     * @returns {Object} Expertise object
     */
    getAvatarExpertise(ethnicity, gender) {
        // This would map to the 8 diverse characters specified
        const expertiseMap = {
            'caucasian_male': { name: 'Jake Thompson', specialty: 'outdoor_growing', experience: 'colorado_expert' },
            'caucasian_female': { name: 'Sarah Mitchell', specialty: 'hydroponics', experience: 'botanist' },
            'african_male': { name: 'Marcus Johnson', specialty: 'organic_cultivation', experience: 'california_expert' },
            'african_female': { name: 'Amara Williams', specialty: 'sustainable_farming', experience: 'educator' },
            'asian_male': { name: 'Hiroshi Tanaka', specialty: 'precision_agriculture', experience: 'technologist' },
            'asian_female': { name: 'Li Wei Chen', specialty: 'traditional_medicine', experience: 'researcher' },
            'hispanic_male': { name: 'Carlos Rodriguez', specialty: 'family_traditions', experience: 'multi_generational' },
            'hispanic_female': { name: 'Isabella Martinez', specialty: 'business', experience: 'entrepreneur' }
        };
        
        return expertiseMap[`${ethnicity}_${gender}`] || { name: 'Unknown', specialty: 'general', experience: 'beginner' };
    }
    
    /**
     * Optimize model based on LOD level
     * @param {Object3D} model - Three.js model
     * @param {number} lodLevel - LOD level (0, 1, 2)
     */
    optimizeModel(model, lodLevel) {
        model.traverse((child) => {
            if (child.isMesh) {
                // Disable shadows on lower LODs
                if (lodLevel > 0) {
                    child.castShadow = false;
                    child.receiveShadow = false;
                }
                
                // Simplify materials on lower LODs
                if (lodLevel === 2 && child.material) {
                    // Use simpler materials for LOD2
                    if (child.material.map) {
                        child.material.normalMap = null;
                        child.material.roughnessMap = null;
                        child.material.metalnessMap = null;
                    }
                }
                
                // Frustum culling
                child.frustumCulled = true;
            }
        });
    }
    
    /**
     * Update performance statistics
     * @param {Object3D} model - Loaded model
     * @param {string} lodLevel - LOD level
     */
    updateStats(model, lodLevel) {
        let triangles = 0;
        
        model.traverse((child) => {
            if (child.isMesh && child.geometry) {
                const geometry = child.geometry;
                if (geometry.index) {
                    triangles += geometry.index.count / 3;
                } else {
                    triangles += geometry.attributes.position.count / 3;
                }
            }
        });
        
        this.stats.totalTriangles += triangles;
        this.stats.loadedAssets++;
        
        console.log(`${lodLevel} loaded: ${triangles.toFixed(0)} triangles`);
    }
    
    /**
     * Get current performance statistics
     * @returns {Object} Performance stats
     */
    getStats() {
        return { ...this.stats };
    }
    
    /**
     * Preload essential assets
     * @returns {Promise} Promise that resolves when preloading is complete
     */
    async preloadEssentials() {
        const essentialAssets = [
            '/assets/models/optimized/cannabis_vegetative_v0_LOD0.glb',
            '/assets/models/optimized/dwc_bucket_LOD0.glb',
            '/assets/models/optimized/outdoor_tile_LOD0.glb',
            '/assets/textures/grass_seamless.jpg'
        ];
        
        const promises = essentialAssets.map(asset => {
            return new Promise((resolve, reject) => {
                if (asset.endsWith('.glb')) {
                          this.gltfLoader.load(asset, resolve, undefined, reject);                } else {
                    this.textureLoader.load(asset, resolve, undefined, reject);
                }
            });
        });
        
        try {
            await Promise.all(promises);
            console.log('Essential assets preloaded');
        } catch (error) {
            console.error('Failed to preload essential assets:', error);
        }
    }
    
    /**
     * Dispose of all loaded assets to free memory
     */
    dispose() {
        this.cache.clear();
        this.dracoLoader.dispose();
        this.ktx2Loader.dispose();
        
        // Reset stats
        this.stats = {
            totalTriangles: 0,
            drawCalls: 0,
            loadedAssets: 0,
            memoryUsage: 0
        };
    }
}

// Export for use in main game
export { CannaVilleAssetLoader };

