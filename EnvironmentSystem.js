/**
 * CannaVille Pro - Environment System
 * Manages indoor DWC and outdoor growing environments with realistic details
 */

import * as THREE from 'three';
import { gsap } from 'gsap';

export class EnvironmentSystem {
    constructor(engine) {
        this.engine = engine;
        this.currentEnvironment = null;
        this.environments = new Map();
        this.environmentObjects = new Map();
        
        // Environment configurations
        this.environmentConfigs = {
            indoor: {
                name: 'Indoor DWC System',
                lighting: {
                    ambient: { color: 0x404040, intensity: 0.2 },
                    directional: { color: 0xffffff, intensity: 0.5 },
                    growLights: [
                        { position: { x: -2, y: 6, z: 0 }, color: 0xff1493, intensity: 2 },
                        { position: { x: 2, y: 6, z: 0 }, color: 0xff1493, intensity: 2 }
                    ]
                },
                atmosphere: {
                    fog: { color: 0x87CEEB, near: 10, far: 50 },
                    humidity: 0.6,
                    temperature: 24,
                    co2: 400
                },
                equipment: {
                    tent: true,
                    dwcBuckets: 6,
                    reservoir: true,
                    ventilation: true,
                    monitoring: true
                }
            },
            outdoor: {
                name: 'Outdoor Cultivation Grid',
                lighting: {
                    ambient: { color: 0x87CEEB, intensity: 0.4 },
                    directional: { color: 0xffffff, intensity: 1.2 },
                    growLights: []
                },
                atmosphere: {
                    fog: { color: 0x87CEEB, near: 50, far: 200 },
                    humidity: 0.5,
                    temperature: 26,
                    co2: 380
                },
                equipment: {
                    soilGrid: { size: 5, spacing: 2 },
                    irrigation: true,
                    fencing: true,
                    weatherProtection: false
                }
            }
        };
        
        this.isInitialized = false;
    }
    
    /**
     * Initialize the environment system
     */
    async init() {
        console.log('🏠 Initializing Environment System...');
        
        try {
            // Create environment models
            await this.createEnvironmentModels();
            
            // Setup environmental effects
            this.setupEnvironmentalEffects();
            
            // Setup dynamic systems
            this.setupDynamicSystems();
            
            this.isInitialized = true;
            console.log('✅ Environment System initialized');
            
        } catch (error) {
            console.error('❌ Failed to initialize Environment System:', error);
            throw error;
        }
    }
    
    /**
     * Create 3D models for both environments
     */
    async createEnvironmentModels() {
        // Create indoor DWC environment
        const indoorEnvironment = await this.createIndoorEnvironment();
        this.environments.set('indoor', indoorEnvironment);
        
        // Create outdoor environment
        const outdoorEnvironment = await this.createOutdoorEnvironment();
        this.environments.set('outdoor', outdoorEnvironment);
        
        console.log('✅ Created environment models');
    }
    
    /**
     * Create indoor DWC hydroponic environment
     */
    async createIndoorEnvironment() {
        const environment = new THREE.Group();
        environment.name = 'IndoorDWC';
        
        // Create grow tent structure
        const tent = this.createGrowTent();
        environment.add(tent);
        
        // Create DWC bucket system
        const dwcSystem = this.createDWCSystem();
        environment.add(dwcSystem);
        
        // Create LED lighting system
        const lightingSystem = this.createLEDLightingSystem();
        environment.add(lightingSystem);
        
        // Create ventilation system
        const ventilation = this.createVentilationSystem();
        environment.add(ventilation);
        
        // Create monitoring equipment
        const monitoring = this.createMonitoringEquipment();
        environment.add(monitoring);
        
        // Create reflective floor
        const floor = this.createReflectiveFloor();
        environment.add(floor);
        
        return environment;
    }
    
    /**
     * Create grow tent structure
     */
    createGrowTent() {
        const tent = new THREE.Group();
        tent.name = 'GrowTent';
        
        // Tent dimensions: 4x4 feet (1.22m x 1.22m)
        const tentSize = { width: 4, height: 6, depth: 4 };
        
        // Create tent frame
        const frameGeometry = new THREE.BoxGeometry(0.05, tentSize.height, 0.05);
        const frameMaterial = new THREE.MeshLambertMaterial({ color: 0x333333 });
        
        // Corner posts
        const corners = [
            { x: -tentSize.width/2, z: -tentSize.depth/2 },
            { x: tentSize.width/2, z: -tentSize.depth/2 },
            { x: tentSize.width/2, z: tentSize.depth/2 },
            { x: -tentSize.width/2, z: tentSize.depth/2 }
        ];
        
        corners.forEach(corner => {
            const post = new THREE.Mesh(frameGeometry, frameMaterial);
            post.position.set(corner.x, tentSize.height/2, corner.z);
            tent.add(post);
        });
        
        // Create mylar walls
        const wallMaterial = new THREE.MeshPhongMaterial({ 
            color: 0xffffff,
            shininess: 100,
            reflectivity: 0.9,
            side: THREE.DoubleSide
        });
        
        // Back wall
        const backWallGeometry = new THREE.PlaneGeometry(tentSize.width, tentSize.height);
        const backWall = new THREE.Mesh(backWallGeometry, wallMaterial);
        backWall.position.set(0, tentSize.height/2, -tentSize.depth/2);
        tent.add(backWall);
        
        // Side walls
        const sideWallGeometry = new THREE.PlaneGeometry(tentSize.depth, tentSize.height);
        
        const leftWall = new THREE.Mesh(sideWallGeometry, wallMaterial);
        leftWall.position.set(-tentSize.width/2, tentSize.height/2, 0);
        leftWall.rotation.y = Math.PI/2;
        tent.add(leftWall);
        
        const rightWall = new THREE.Mesh(sideWallGeometry, wallMaterial);
        rightWall.position.set(tentSize.width/2, tentSize.height/2, 0);
        rightWall.rotation.y = -Math.PI/2;
        tent.add(rightWall);
        
        // Ceiling
        const ceilingGeometry = new THREE.PlaneGeometry(tentSize.width, tentSize.depth);
        const ceiling = new THREE.Mesh(ceilingGeometry, wallMaterial);
        ceiling.position.set(0, tentSize.height, 0);
        ceiling.rotation.x = -Math.PI/2;
        tent.add(ceiling);
        
        return tent;
    }
    
    /**
     * Create DWC (Deep Water Culture) bucket system
     */
    createDWCSystem() {
        const dwcSystem = new THREE.Group();
        dwcSystem.name = 'DWCSystem';
        
        // Create 6 buckets in 2x3 configuration
        const bucketPositions = [
            { x: -1.5, z: -1 }, { x: 0, z: -1 }, { x: 1.5, z: -1 },
            { x: -1.5, z: 1 }, { x: 0, z: 1 }, { x: 1.5, z: 1 }
        ];
        
        bucketPositions.forEach((pos, index) => {
            const bucket = this.createDWCBucket();
            bucket.position.set(pos.x, 0, pos.z);
            bucket.userData = { bucketId: index, type: 'dwc_bucket' };
            dwcSystem.add(bucket);
        });
        
        // Create central reservoir
        const reservoir = this.createReservoir();
        reservoir.position.set(0, -0.5, 0);
        dwcSystem.add(reservoir);
        
        // Create PVC piping system
        const pipingSystem = this.createPipingSystem(bucketPositions);
        dwcSystem.add(pipingSystem);
        
        // Create air pump and stones
        const airSystem = this.createAirSystem(bucketPositions);
        dwcSystem.add(airSystem);
        
        return dwcSystem;
    }
    
    /**
     * Create individual DWC bucket
     */
    createDWCBucket() {
        const bucket = new THREE.Group();
        
        // Bucket container (5 gallon)
        const bucketGeometry = new THREE.CylinderGeometry(0.3, 0.25, 0.4, 16);
        const bucketMaterial = new THREE.MeshLambertMaterial({ color: 0x222222 });
        const bucketMesh = new THREE.Mesh(bucketGeometry, bucketMaterial);
        bucketMesh.position.y = 0.2;
        bucket.add(bucketMesh);
        
        // Net pot lid
        const lidGeometry = new THREE.CylinderGeometry(0.32, 0.32, 0.02, 16);
        const lidMaterial = new THREE.MeshLambertMaterial({ color: 0x444444 });
        const lid = new THREE.Mesh(lidGeometry, lidMaterial);
        lid.position.y = 0.41;
        bucket.add(lid);
        
        // Net pot hole
        const holeGeometry = new THREE.CylinderGeometry(0.08, 0.08, 0.03, 8);
        const holeMaterial = new THREE.MeshLambertMaterial({ color: 0x000000 });
        const hole = new THREE.Mesh(holeGeometry, holeMaterial);
        hole.position.y = 0.42;
        bucket.add(hole);
        
        // Water level indicator
        const waterGeometry = new THREE.CylinderGeometry(0.28, 0.23, 0.3, 16);
        const waterMaterial = new THREE.MeshPhongMaterial({ 
            color: 0x4a90e2,
            transparent: true,
            opacity: 0.6,
            shininess: 100
        });
        const water = new THREE.Mesh(waterGeometry, waterMaterial);
        water.position.y = 0.15;
        bucket.add(water);
        
        // Air stone bubbles effect
        this.createBubbleEffect(bucket, water.position);
        
        return bucket;
    }
    
    /**
     * Create bubble effect for air stones
     */
    createBubbleEffect(parent, waterPosition) {
        const bubbles = new THREE.Group();
        bubbles.name = 'Bubbles';
        
        // Create bubble particles
        for (let i = 0; i < 10; i++) {
            const bubbleGeometry = new THREE.SphereGeometry(0.005 + Math.random() * 0.01, 4, 4);
            const bubbleMaterial = new THREE.MeshBasicMaterial({ 
                color: 0xffffff,
                transparent: true,
                opacity: 0.3
            });
            
            const bubble = new THREE.Mesh(bubbleGeometry, bubbleMaterial);
            bubble.position.set(
                (Math.random() - 0.5) * 0.4,
                waterPosition.y - 0.1,
                (Math.random() - 0.5) * 0.4
            );
            
            bubbles.add(bubble);
            
            // Animate bubbles rising
            this.animateBubble(bubble, waterPosition);
        }
        
        parent.add(bubbles);
    }
    
    /**
     * Animate individual bubble
     */
    animateBubble(bubble, waterPosition) {
        const animateBubbleRise = () => {
            // Reset bubble position
            bubble.position.y = waterPosition.y - 0.1;
            bubble.position.x = (Math.random() - 0.5) * 0.4;
            bubble.position.z = (Math.random() - 0.5) * 0.4;
            bubble.material.opacity = 0.3;
            
            // Animate to surface
            gsap.to(bubble.position, {
                duration: 2 + Math.random() * 2,
                y: waterPosition.y + 0.2,
                ease: 'power1.out',
                onComplete: animateBubbleRise
            });
            
            gsap.to(bubble.material, {
                duration: 2 + Math.random() * 2,
                opacity: 0,
                ease: 'power1.out'
            });
        };
        
        // Start with random delay
        setTimeout(animateBubbleRise, Math.random() * 2000);
    }
    
    /**
     * Create central reservoir
     */
    createReservoir() {
        const reservoir = new THREE.Group();
        
        // Main tank
        const tankGeometry = new THREE.BoxGeometry(1, 0.6, 0.8);
        const tankMaterial = new THREE.MeshLambertMaterial({ color: 0x333333 });
        const tank = new THREE.Mesh(tankGeometry, tankMaterial);
        reservoir.add(tank);
        
        // Water pump
        const pumpGeometry = new THREE.BoxGeometry(0.2, 0.15, 0.15);
        const pumpMaterial = new THREE.MeshLambertMaterial({ color: 0x666666 });
        const pump = new THREE.Mesh(pumpGeometry, pumpMaterial);
        pump.position.set(0.3, 0.1, 0);
        reservoir.add(pump);
        
        return reservoir;
    }
    
    /**
     * Create PVC piping system
     */
    createPipingSystem(bucketPositions) {
        const piping = new THREE.Group();
        piping.name = 'PipingSystem';
        
        // Main supply line
        const mainLineGeometry = new THREE.CylinderGeometry(0.02, 0.02, 4, 8);
        const pipeMaterial = new THREE.MeshLambertMaterial({ color: 0xeeeeee });
        
        const mainLine = new THREE.Mesh(mainLineGeometry, pipeMaterial);
        mainLine.position.set(0, 0.1, 0);
        mainLine.rotation.z = Math.PI/2;
        piping.add(mainLine);
        
        // Connect to each bucket
        bucketPositions.forEach(pos => {
            const connectionGeometry = new THREE.CylinderGeometry(0.015, 0.015, 1, 8);
            const connection = new THREE.Mesh(connectionGeometry, pipeMaterial);
            connection.position.set(pos.x, 0.1, pos.z);
            connection.rotation.x = Math.PI/2;
            piping.add(connection);
        });
        
        return piping;
    }
    
    /**
     * Create air system with pump and stones
     */
    createAirSystem(bucketPositions) {
        const airSystem = new THREE.Group();
        airSystem.name = 'AirSystem';
        
        // Air pump
        const pumpGeometry = new THREE.BoxGeometry(0.3, 0.2, 0.2);
        const pumpMaterial = new THREE.MeshLambertMaterial({ color: 0x444444 });
        const airPump = new THREE.Mesh(pumpGeometry, pumpMaterial);
        airPump.position.set(-2.5, 0.5, 0);
        airSystem.add(airPump);
        
        // Air stones in each bucket
        bucketPositions.forEach(pos => {
            const stoneGeometry = new THREE.SphereGeometry(0.03, 8, 6);
            const stoneMaterial = new THREE.MeshLambertMaterial({ color: 0x888888 });
            const airStone = new THREE.Mesh(stoneGeometry, stoneMaterial);
            airStone.position.set(pos.x, 0.05, pos.z);
            airSystem.add(airStone);
        });
        
        return airSystem;
    }
    
    /**
     * Create LED lighting system
     */
    createLEDLightingSystem() {
        const lighting = new THREE.Group();
        lighting.name = 'LEDLighting';
        
        // Create two quantum board LED panels
        const ledPositions = [
            { x: -1.5, y: 5, z: 0 },
            { x: 1.5, y: 5, z: 0 }
        ];
        
        ledPositions.forEach(pos => {
            const ledPanel = this.createLEDPanel();
            ledPanel.position.set(pos.x, pos.y, pos.z);
            lighting.add(ledPanel);
        });
        
        return lighting;
    }
    
    /**
     * Create individual LED panel
     */
    createLEDPanel() {
        const panel = new THREE.Group();
        
        // Heat sink
        const heatsinkGeometry = new THREE.BoxGeometry(1.2, 0.1, 0.6);
        const heatsinkMaterial = new THREE.MeshLambertMaterial({ color: 0x666666 });
        const heatsink = new THREE.Mesh(heatsinkGeometry, heatsinkMaterial);
        panel.add(heatsink);
        
        // LED array
        const ledGeometry = new THREE.PlaneGeometry(1, 0.5);
        const ledMaterial = new THREE.MeshBasicMaterial({ 
            color: 0xff1493,
            emissive: 0xff1493,
            emissiveIntensity: 0.5
        });
        const ledArray = new THREE.Mesh(ledGeometry, ledMaterial);
        ledArray.position.y = -0.06;
        ledArray.rotation.x = -Math.PI/2;
        panel.add(ledArray);
        
        // Individual LED points
        for (let x = 0; x < 20; x++) {
            for (let y = 0; y < 10; y++) {
                const ledPoint = new THREE.Mesh(
                    new THREE.SphereGeometry(0.01, 4, 4),
                    new THREE.MeshBasicMaterial({ 
                        color: 0xff69b4,
                        emissive: 0xff69b4,
                        emissiveIntensity: 0.3
                    })
                );
                
                ledPoint.position.set(
                    (x - 9.5) * 0.05,
                    -0.07,
                    (y - 4.5) * 0.05
                );
                
                panel.add(ledPoint);
            }
        }
        
        return panel;
    }
    
    /**
     * Create ventilation system
     */
    createVentilationSystem() {
        const ventilation = new THREE.Group();
        ventilation.name = 'Ventilation';
        
        // Exhaust fan
        const fanGeometry = new THREE.CylinderGeometry(0.3, 0.3, 0.2, 16);
        const fanMaterial = new THREE.MeshLambertMaterial({ color: 0x333333 });
        const exhaustFan = new THREE.Mesh(fanGeometry, fanMaterial);
        exhaustFan.position.set(0, 5.5, -2);
        exhaustFan.rotation.x = Math.PI/2;
        ventilation.add(exhaustFan);
        
        // Intake fan
        const intakeFan = new THREE.Mesh(fanGeometry, fanMaterial);
        intakeFan.position.set(0, 1, 2);
        intakeFan.rotation.x = Math.PI/2;
        ventilation.add(intakeFan);
        
        // Ducting
        const ductGeometry = new THREE.CylinderGeometry(0.15, 0.15, 2, 8);
        const ductMaterial = new THREE.MeshLambertMaterial({ color: 0x888888 });
        const duct = new THREE.Mesh(ductGeometry, ductMaterial);
        duct.position.set(0, 4, -2);
        duct.rotation.x = Math.PI/2;
        ventilation.add(duct);
        
        return ventilation;
    }
    
    /**
     * Create monitoring equipment
     */
    createMonitoringEquipment() {
        const monitoring = new THREE.Group();
        monitoring.name = 'Monitoring';
        
        // pH meter
        const phMeterGeometry = new THREE.BoxGeometry(0.1, 0.2, 0.05);
        const meterMaterial = new THREE.MeshLambertMaterial({ color: 0x222222 });
        const phMeter = new THREE.Mesh(phMeterGeometry, meterMaterial);
        phMeter.position.set(-1.8, 1, 0);
        monitoring.add(phMeter);
        
        // Temperature/humidity sensor
        const sensorGeometry = new THREE.BoxGeometry(0.08, 0.08, 0.03);
        const sensor = new THREE.Mesh(sensorGeometry, meterMaterial);
        sensor.position.set(1.8, 2, 0);
        monitoring.add(sensor);
        
        // Control panel
        const panelGeometry = new THREE.BoxGeometry(0.3, 0.4, 0.05);
        const panelMaterial = new THREE.MeshLambertMaterial({ color: 0x444444 });
        const controlPanel = new THREE.Mesh(panelGeometry, panelMaterial);
        controlPanel.position.set(-1.8, 2, -1.8);
        monitoring.add(controlPanel);
        
        return monitoring;
    }
    
    /**
     * Create reflective floor
     */
    createReflectiveFloor() {
        const floorGeometry = new THREE.PlaneGeometry(6, 6);
        const floorMaterial = new THREE.MeshPhongMaterial({ 
            color: 0xffffff,
            shininess: 100,
            reflectivity: 0.3
        });
        
        const floor = new THREE.Mesh(floorGeometry, floorMaterial);
        floor.rotation.x = -Math.PI/2;
        floor.position.y = -0.01;
        floor.receiveShadow = true;
        
        return floor;
    }
    
    /**
     * Create outdoor cultivation environment
     */
    async createOutdoorEnvironment() {
        const environment = new THREE.Group();
        environment.name = 'OutdoorCultivation';
        
        // Create ground plane
        const ground = this.createOutdoorGround();
        environment.add(ground);
        
        // Create 5x5 planting grid
        const plantingGrid = this.createPlantingGrid();
        environment.add(plantingGrid);
        
        // Create perimeter fencing
        const fencing = this.createPerimeterFencing();
        environment.add(fencing);
        
        // Create irrigation system
        const irrigation = this.createIrrigationSystem();
        environment.add(irrigation);
        
        // Create tool shed
        const toolShed = this.createToolShed();
        environment.add(toolShed);
        
        // Create environmental elements
        const environmentalElements = this.createEnvironmentalElements();
        environment.add(environmentalElements);
        
        return environment;
    }
    
    /**
     * Create outdoor ground with grass texture
     */
    createOutdoorGround() {
        const groundGeometry = new THREE.PlaneGeometry(50, 50, 32, 32);
        
        // Create grass texture
        const grassTexture = this.createGrassTexture();
        const groundMaterial = new THREE.MeshLambertMaterial({ 
            map: grassTexture,
            color: 0x4a7c59
        });
        
        const ground = new THREE.Mesh(groundGeometry, groundMaterial);
        ground.rotation.x = -Math.PI/2;
        ground.receiveShadow = true;
        
        // Add some terrain variation
        const positions = ground.geometry.attributes.position.array;
        for (let i = 0; i < positions.length; i += 3) {
            positions[i + 1] = Math.random() * 0.1; // Y position (height)
        }
        ground.geometry.attributes.position.needsUpdate = true;
        ground.geometry.computeVertexNormals();
        
        return ground;
    }
    
    /**
     * Create grass texture
     */
    createGrassTexture() {
        const canvas = document.createElement('canvas');
        canvas.width = 512;
        canvas.height = 512;
        const ctx = canvas.getContext('2d');
        
        // Create grass pattern
        ctx.fillStyle = '#4a7c59';
        ctx.fillRect(0, 0, 512, 512);
        
        // Add grass blades
        for (let i = 0; i < 1000; i++) {
            const x = Math.random() * 512;
            const y = Math.random() * 512;
            const length = 5 + Math.random() * 10;
            
            ctx.strokeStyle = `hsl(${100 + Math.random() * 40}, 60%, ${30 + Math.random() * 20}%)`;
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.moveTo(x, y);
            ctx.lineTo(x + (Math.random() - 0.5) * 4, y - length);
            ctx.stroke();
        }
        
        const texture = new THREE.CanvasTexture(canvas);
        texture.wrapS = THREE.RepeatWrapping;
        texture.wrapT = THREE.RepeatWrapping;
        texture.repeat.set(10, 10);
        
        return texture;
    }
    
    /**
     * Create 5x5 planting grid
     */
    createPlantingGrid() {
        const grid = new THREE.Group();
        grid.name = 'PlantingGrid';
        
        const gridSize = 5;
        const spacing = 2;
        const startX = -(gridSize - 1) * spacing / 2;
        const startZ = -(gridSize - 1) * spacing / 2;
        
        for (let x = 0; x < gridSize; x++) {
            for (let z = 0; z < gridSize; z++) {
                const hole = this.createPlantingHole();
                hole.position.set(
                    startX + x * spacing,
                    0,
                    startZ + z * spacing
                );
                hole.userData = { 
                    gridX: x, 
                    gridZ: z, 
                    type: 'planting_hole',
                    occupied: false 
                };
                grid.add(hole);
            }
        }
        
        return grid;
    }
    
    /**
     * Create individual planting hole
     */
    createPlantingHole() {
        const hole = new THREE.Group();
        
        // Excavated soil
        const holeGeometry = new THREE.CylinderGeometry(0.4, 0.3, 0.3, 16);
        const soilMaterial = new THREE.MeshLambertMaterial({ color: 0x8b4513 });
        const holeMesh = new THREE.Mesh(holeGeometry, soilMaterial);
        holeMesh.position.y = -0.15;
        hole.add(holeMesh);
        
        // Soil mound around hole
        const moundGeometry = new THREE.RingGeometry(0.4, 0.6, 16);
        const mound = new THREE.Mesh(moundGeometry, soilMaterial);
        mound.rotation.x = -Math.PI/2;
        mound.position.y = 0.02;
        hole.add(mound);
        
        // Add some scattered soil particles
        for (let i = 0; i < 10; i++) {
            const particleGeometry = new THREE.SphereGeometry(0.01 + Math.random() * 0.02, 4, 4);
            const particle = new THREE.Mesh(particleGeometry, soilMaterial);
            particle.position.set(
                (Math.random() - 0.5) * 1.2,
                0.01,
                (Math.random() - 0.5) * 1.2
            );
            hole.add(particle);
        }
        
        return hole;
    }
    
    /**
     * Create perimeter fencing
     */
    createPerimeterFencing() {
        const fencing = new THREE.Group();
        fencing.name = 'PerimeterFencing';
        
        const fenceHeight = 2;
        const fenceLength = 20;
        
        // Create fence posts and panels
        const postGeometry = new THREE.CylinderGeometry(0.05, 0.05, fenceHeight, 8);
        const postMaterial = new THREE.MeshLambertMaterial({ color: 0x8b4513 });
        
        const panelGeometry = new THREE.PlaneGeometry(2, fenceHeight);
        const panelMaterial = new THREE.MeshLambertMaterial({ 
            color: 0x654321,
            transparent: true,
            opacity: 0.8
        });
        
        // Front and back fences
        for (let i = 0; i <= fenceLength / 2; i++) {
            const x = -fenceLength/2 + i * 2;
            
            // Front fence
            const frontPost = new THREE.Mesh(postGeometry, postMaterial);
            frontPost.position.set(x, fenceHeight/2, fenceLength/2);
            fencing.add(frontPost);
            
            if (i < fenceLength / 2) {
                const frontPanel = new THREE.Mesh(panelGeometry, panelMaterial);
                frontPanel.position.set(x + 1, fenceHeight/2, fenceLength/2);
                fencing.add(frontPanel);
            }
            
            // Back fence
            const backPost = new THREE.Mesh(postGeometry, postMaterial);
            backPost.position.set(x, fenceHeight/2, -fenceLength/2);
            fencing.add(backPost);
            
            if (i < fenceLength / 2) {
                const backPanel = new THREE.Mesh(panelGeometry, panelMaterial);
                backPanel.position.set(x + 1, fenceHeight/2, -fenceLength/2);
                fencing.add(backPanel);
            }
        }
        
        // Side fences
        for (let i = 0; i <= fenceLength / 2; i++) {
            const z = -fenceLength/2 + i * 2;
            
            // Left fence
            const leftPost = new THREE.Mesh(postGeometry, postMaterial);
            leftPost.position.set(-fenceLength/2, fenceHeight/2, z);
            fencing.add(leftPost);
            
            if (i < fenceLength / 2) {
                const leftPanel = new THREE.Mesh(panelGeometry, panelMaterial);
                leftPanel.position.set(-fenceLength/2, fenceHeight/2, z + 1);
                leftPanel.rotation.y = Math.PI/2;
                fencing.add(leftPanel);
            }
            
            // Right fence
            const rightPost = new THREE.Mesh(postGeometry, postMaterial);
            rightPost.position.set(fenceLength/2, fenceHeight/2, z);
            fencing.add(rightPost);
            
            if (i < fenceLength / 2) {
                const rightPanel = new THREE.Mesh(panelGeometry, panelMaterial);
                rightPanel.position.set(fenceLength/2, fenceHeight/2, z + 1);
                rightPanel.rotation.y = Math.PI/2;
                fencing.add(rightPanel);
            }
        }
        
        return fencing;
    }
    
    /**
     * Create irrigation system
     */
    createIrrigationSystem() {
        const irrigation = new THREE.Group();
        irrigation.name = 'IrrigationSystem';
        
        // Main water line
        const mainLineGeometry = new THREE.CylinderGeometry(0.03, 0.03, 12, 8);
        const pipeMaterial = new THREE.MeshLambertMaterial({ color: 0x333333 });
        
        const mainLine = new THREE.Mesh(mainLineGeometry, pipeMaterial);
        mainLine.position.set(0, 0.1, -6);
        mainLine.rotation.z = Math.PI/2;
        irrigation.add(mainLine);
        
        // Drip lines to each planting hole
        const gridSize = 5;
        const spacing = 2;
        const startX = -(gridSize - 1) * spacing / 2;
        const startZ = -(gridSize - 1) * spacing / 2;
        
        for (let x = 0; x < gridSize; x++) {
            for (let z = 0; z < gridSize; z++) {
                const dripLineGeometry = new THREE.CylinderGeometry(0.01, 0.01, 6, 6);
                const dripLine = new THREE.Mesh(dripLineGeometry, pipeMaterial);
                
                const posX = startX + x * spacing;
                const posZ = startZ + z * spacing;
                
                dripLine.position.set(posX, 0.05, posZ - 3);
                dripLine.rotation.x = Math.PI/2;
                irrigation.add(dripLine);
                
                // Drip emitter
                const emitterGeometry = new THREE.SphereGeometry(0.02, 6, 6);
                const emitter = new THREE.Mesh(emitterGeometry, pipeMaterial);
                emitter.position.set(posX, 0.05, posZ);
                irrigation.add(emitter);
            }
        }
        
        return irrigation;
    }
    
    /**
     * Create tool shed
     */
    createToolShed() {
        const shed = new THREE.Group();
        shed.name = 'ToolShed';
        
        // Shed structure
        const shedGeometry = new THREE.BoxGeometry(3, 2.5, 2);
        const shedMaterial = new THREE.MeshLambertMaterial({ color: 0x8b4513 });
        const shedMesh = new THREE.Mesh(shedGeometry, shedMaterial);
        shedMesh.position.set(-8, 1.25, -8);
        shed.add(shedMesh);
        
        // Roof
        const roofGeometry = new THREE.ConeGeometry(2.2, 1, 4);
        const roofMaterial = new THREE.MeshLambertMaterial({ color: 0x654321 });
        const roof = new THREE.Mesh(roofGeometry, roofMaterial);
        roof.position.set(-8, 3, -8);
        roof.rotation.y = Math.PI/4;
        shed.add(roof);
        
        // Door
        const doorGeometry = new THREE.PlaneGeometry(0.8, 2);
        const doorMaterial = new THREE.MeshLambertMaterial({ color: 0x4a4a4a });
        const door = new THREE.Mesh(doorGeometry, doorMaterial);
        door.position.set(-6.49, 1, -8);
        shed.add(door);
        
        return shed;
    }
    
    /**
     * Create environmental elements (trees, rocks, etc.)
     */
    createEnvironmentalElements() {
        const elements = new THREE.Group();
        elements.name = 'EnvironmentalElements';
        
        // Add some background trees
        for (let i = 0; i < 8; i++) {
            const tree = this.createSimpleTree();
            const angle = (i / 8) * Math.PI * 2;
            const distance = 15 + Math.random() * 10;
            
            tree.position.set(
                Math.cos(angle) * distance,
                0,
                Math.sin(angle) * distance
            );
            
            elements.add(tree);
        }
        
        // Add some rocks
        for (let i = 0; i < 5; i++) {
            const rock = this.createRock();
            rock.position.set(
                (Math.random() - 0.5) * 30,
                0,
                (Math.random() - 0.5) * 30
            );
            elements.add(rock);
        }
        
        return elements;
    }
    
    /**
     * Create simple tree
     */
    createSimpleTree() {
        const tree = new THREE.Group();
        
        // Trunk
        const trunkGeometry = new THREE.CylinderGeometry(0.2, 0.3, 4, 8);
        const trunkMaterial = new THREE.MeshLambertMaterial({ color: 0x8b4513 });
        const trunk = new THREE.Mesh(trunkGeometry, trunkMaterial);
        trunk.position.y = 2;
        tree.add(trunk);
        
        // Foliage
        const foliageGeometry = new THREE.SphereGeometry(2, 8, 6);
        const foliageMaterial = new THREE.MeshLambertMaterial({ color: 0x228b22 });
        const foliage = new THREE.Mesh(foliageGeometry, foliageMaterial);
        foliage.position.y = 5;
        tree.add(foliage);
        
        return tree;
    }
    
    /**
     * Create rock
     */
    createRock() {
        const rockGeometry = new THREE.SphereGeometry(0.5 + Math.random() * 0.5, 6, 4);
        const rockMaterial = new THREE.MeshLambertMaterial({ color: 0x696969 });
        const rock = new THREE.Mesh(rockGeometry, rockMaterial);
        
        // Deform the sphere to make it more rock-like
        const positions = rock.geometry.attributes.position.array;
        for (let i = 0; i < positions.length; i += 3) {
            const scale = 0.8 + Math.random() * 0.4;
            positions[i] *= scale;
            positions[i + 1] *= scale;
            positions[i + 2] *= scale;
        }
        rock.geometry.attributes.position.needsUpdate = true;
        rock.geometry.computeVertexNormals();
        
        return rock;
    }
    
    /**
     * Setup environmental effects
     */
    setupEnvironmentalEffects() {
        // Wind effects for outdoor environment
        this.setupWindEffects();
        
        // Day/night cycle
        this.setupDayNightCycle();
        
        // Weather effects
        this.setupWeatherEffects();
    }
    
    /**
     * Setup wind effects
     */
    setupWindEffects() {
        // Wind will affect plant movement and particle systems
        this.windStrength = 0.5;
        this.windDirection = new THREE.Vector3(1, 0, 0);
        
        // Animate wind direction
        const animateWind = () => {
            gsap.to(this.windDirection, {
                duration: 10 + Math.random() * 10,
                x: (Math.random() - 0.5) * 2,
                z: (Math.random() - 0.5) * 2,
                ease: 'sine.inOut',
                onComplete: animateWind
            });
        };
        
        animateWind();
    }
    
    /**
     * Setup day/night cycle
     */
    setupDayNightCycle() {
        this.timeOfDay = 0.5; // 0 = midnight, 0.5 = noon, 1 = midnight
        this.dayDuration = 120; // 2 minutes per day cycle
        
        const updateDayNight = () => {
            this.timeOfDay += 1 / (this.dayDuration * 60); // 60 FPS
            if (this.timeOfDay > 1) this.timeOfDay = 0;
            
            // Update sun position and intensity
            this.updateSunLighting();
            
            requestAnimationFrame(updateDayNight);
        };
        
        updateDayNight();
    }
    
    /**
     * Update sun lighting based on time of day
     */
    updateSunLighting() {
        if (!this.engine.scene) return;
        
        // Find directional light (sun)
        const sun = this.engine.scene.children.find(child => 
            child.type === 'DirectionalLight'
        );
        
        if (sun) {
            // Calculate sun position
            const sunAngle = this.timeOfDay * Math.PI * 2;
            const sunHeight = Math.sin(sunAngle);
            const sunX = Math.cos(sunAngle) * 20;
            const sunY = Math.max(0.1, sunHeight * 20);
            
            sun.position.set(sunX, sunY, 5);
            
            // Adjust intensity based on time of day
            const intensity = Math.max(0.1, sunHeight);
            sun.intensity = intensity;
            
            // Adjust color temperature
            const colorTemp = sunHeight > 0 ? 0xffffff : 0x4169e1; // White during day, blue at night
            sun.color.setHex(colorTemp);
        }
        
        // Update fog color
        if (this.engine.scene.fog) {
            const fogColor = this.timeOfDay > 0.25 && this.timeOfDay < 0.75 ? 0x87CEEB : 0x191970;
            this.engine.scene.fog.color.setHex(fogColor);
        }
    }
    
    /**
     * Setup weather effects
     */
    setupWeatherEffects() {
        this.weatherState = 'clear'; // clear, cloudy, rain
        this.weatherTransitionTime = 30; // seconds
        
        // Random weather changes
        setInterval(() => {
            this.changeWeather();
        }, this.weatherTransitionTime * 1000);
    }
    
    /**
     * Change weather conditions
     */
    changeWeather() {
        const weatherTypes = ['clear', 'cloudy', 'rain'];
        const newWeather = weatherTypes[Math.floor(Math.random() * weatherTypes.length)];
        
        if (newWeather !== this.weatherState) {
            console.log(`🌤️ Weather changing to: ${newWeather}`);
            this.weatherState = newWeather;
            
            // Apply weather effects
            this.applyWeatherEffects(newWeather);
        }
    }
    
    /**
     * Apply weather effects
     */
    applyWeatherEffects(weather) {
        switch (weather) {
            case 'clear':
                this.clearWeatherEffects();
                break;
            case 'cloudy':
                this.addCloudyEffects();
                break;
            case 'rain':
                this.addRainEffects();
                break;
        }
    }
    
    /**
     * Clear weather effects
     */
    clearWeatherEffects() {
        // Remove rain particles
        const rain = this.engine.scene.getObjectByName('RainSystem');
        if (rain) {
            this.engine.scene.remove(rain);
        }
        
        // Brighten lighting
        const sun = this.engine.scene.children.find(child => 
            child.type === 'DirectionalLight'
        );
        if (sun) {
            gsap.to(sun, { duration: 5, intensity: 1.2 });
        }
    }
    
    /**
     * Add cloudy effects
     */
    addCloudyEffects() {
        // Dim lighting
        const sun = this.engine.scene.children.find(child => 
            child.type === 'DirectionalLight'
        );
        if (sun) {
            gsap.to(sun, { duration: 5, intensity: 0.6 });
        }
        
        // Increase fog density
        if (this.engine.scene.fog) {
            gsap.to(this.engine.scene.fog, { duration: 5, near: 30, far: 100 });
        }
    }
    
    /**
     * Add rain effects
     */
    addRainEffects() {
        // Create rain particle system
        const rainSystem = new THREE.Group();
        rainSystem.name = 'RainSystem';
        
        const rainCount = 1000;
        const rainGeometry = new THREE.BufferGeometry();
        const rainPositions = new Float32Array(rainCount * 3);
        const rainVelocities = new Float32Array(rainCount * 3);
        
        for (let i = 0; i < rainCount; i++) {
            const i3 = i * 3;
            rainPositions[i3] = (Math.random() - 0.5) * 100;
            rainPositions[i3 + 1] = Math.random() * 50 + 20;
            rainPositions[i3 + 2] = (Math.random() - 0.5) * 100;
            
            rainVelocities[i3] = 0;
            rainVelocities[i3 + 1] = -10 - Math.random() * 5;
            rainVelocities[i3 + 2] = 0;
        }
        
        rainGeometry.setAttribute('position', new THREE.BufferAttribute(rainPositions, 3));
        
        const rainMaterial = new THREE.PointsMaterial({
            color: 0x87CEEB,
            size: 0.1,
            transparent: true,
            opacity: 0.6
        });
        
        const rain = new THREE.Points(rainGeometry, rainMaterial);
        rainSystem.add(rain);
        
        this.engine.scene.add(rainSystem);
        
        // Animate rain
        const animateRain = () => {
            const positions = rain.geometry.attributes.position.array;
            
            for (let i = 0; i < rainCount; i++) {
                const i3 = i * 3;
                positions[i3 + 1] += rainVelocities[i3 + 1] * 0.016; // 60 FPS
                
                if (positions[i3 + 1] < 0) {
                    positions[i3 + 1] = 50;
                    positions[i3] = (Math.random() - 0.5) * 100;
                    positions[i3 + 2] = (Math.random() - 0.5) * 100;
                }
            }
            
            rain.geometry.attributes.position.needsUpdate = true;
            
            if (this.weatherState === 'rain') {
                requestAnimationFrame(animateRain);
            }
        };
        
        animateRain();
        
        // Dim lighting
        const sun = this.engine.scene.children.find(child => 
            child.type === 'DirectionalLight'
        );
        if (sun) {
            gsap.to(sun, { duration: 5, intensity: 0.3 });
        }
    }
    
    /**
     * Setup dynamic systems
     */
    setupDynamicSystems() {
        // Equipment monitoring and alerts
        this.setupEquipmentMonitoring();
        
        // Environmental parameter tracking
        this.setupEnvironmentalTracking();
    }
    
    /**
     * Setup equipment monitoring
     */
    setupEquipmentMonitoring() {
        this.equipmentStatus = {
            pumps: { status: 'running', efficiency: 0.95 },
            lights: { status: 'on', intensity: 1.0 },
            ventilation: { status: 'running', airflow: 0.8 },
            sensors: { status: 'active', accuracy: 0.98 }
        };
        
        // Simulate equipment wear and maintenance needs
        setInterval(() => {
            this.updateEquipmentStatus();
        }, 10000); // Update every 10 seconds
    }
    
    /**
     * Update equipment status
     */
    updateEquipmentStatus() {
        // Simulate gradual efficiency degradation
        Object.keys(this.equipmentStatus).forEach(equipment => {
            const status = this.equipmentStatus[equipment];
            if (status.efficiency) {
                status.efficiency = Math.max(0.7, status.efficiency - 0.001);
            }
            if (status.accuracy) {
                status.accuracy = Math.max(0.9, status.accuracy - 0.0005);
            }
        });
        
        // Random equipment issues
        if (Math.random() < 0.01) { // 1% chance per update
            this.simulateEquipmentIssue();
        }
    }
    
    /**
     * Simulate equipment issue
     */
    simulateEquipmentIssue() {
        const equipmentList = Object.keys(this.equipmentStatus);
        const equipment = equipmentList[Math.floor(Math.random() * equipmentList.length)];
        
        console.log(`⚠️ Equipment issue detected: ${equipment}`);
        
        // This would trigger UI alerts in a full implementation
    }
    
    /**
     * Setup environmental tracking
     */
    setupEnvironmentalTracking() {
        this.environmentalData = {
            temperature: 24,
            humidity: 0.6,
            co2: 400,
            ph: 6.5,
            ec: 1.2, // Electrical conductivity
            lightIntensity: 800 // PPFD
        };
        
        // Simulate environmental fluctuations
        setInterval(() => {
            this.updateEnvironmentalData();
        }, 5000); // Update every 5 seconds
    }
    
    /**
     * Update environmental data
     */
    updateEnvironmentalData() {
        // Simulate realistic fluctuations
        this.environmentalData.temperature += (Math.random() - 0.5) * 0.5;
        this.environmentalData.humidity += (Math.random() - 0.5) * 0.02;
        this.environmentalData.co2 += (Math.random() - 0.5) * 10;
        this.environmentalData.ph += (Math.random() - 0.5) * 0.1;
        this.environmentalData.ec += (Math.random() - 0.5) * 0.05;
        
        // Keep values within realistic ranges
        this.environmentalData.temperature = Math.max(18, Math.min(30, this.environmentalData.temperature));
        this.environmentalData.humidity = Math.max(0.3, Math.min(0.8, this.environmentalData.humidity));
        this.environmentalData.co2 = Math.max(300, Math.min(1500, this.environmentalData.co2));
        this.environmentalData.ph = Math.max(5.5, Math.min(7.5, this.environmentalData.ph));
        this.environmentalData.ec = Math.max(0.8, Math.min(2.0, this.environmentalData.ec));
        
        // Adjust light intensity based on time of day
        if (this.currentEnvironment === 'outdoor') {
            const sunIntensity = Math.max(0, Math.sin(this.timeOfDay * Math.PI * 2));
            this.environmentalData.lightIntensity = sunIntensity * 1200;
        }
    }
    
    /**
     * Load specific environment
     */
    async loadEnvironment(environmentType) {
        console.log(`🏠 Loading ${environmentType} environment...`);
        
        // Clear current environment
        if (this.currentEnvironment) {
            const currentEnv = this.environments.get(this.currentEnvironment);
            if (currentEnv) {
                this.engine.scene.remove(currentEnv);
            }
        }
        
        // Load new environment
        const environment = this.environments.get(environmentType);
        if (environment) {
            this.engine.scene.add(environment);
            this.currentEnvironment = environmentType;
            
            // Apply environment-specific settings
            this.applyEnvironmentSettings(environmentType);
            
            console.log(`✅ Loaded ${environmentType} environment`);
        } else {
            throw new Error(`Environment ${environmentType} not found`);
        }
    }
    
    /**
     * Apply environment-specific settings
     */
    applyEnvironmentSettings(environmentType) {
        const config = this.environmentConfigs[environmentType];
        if (!config) return;
        
        // Update lighting
        this.updateEnvironmentLighting(config.lighting);
        
        // Update atmosphere
        this.updateAtmosphere(config.atmosphere);
        
        // Update camera position for environment
        this.updateCameraForEnvironment(environmentType);
    }
    
    /**
     * Update environment lighting
     */
    updateEnvironmentLighting(lightingConfig) {
        // Update ambient light
        const ambientLight = this.engine.scene.children.find(child => 
            child.type === 'AmbientLight'
        );
        if (ambientLight) {
            ambientLight.color.setHex(lightingConfig.ambient.color);
            ambientLight.intensity = lightingConfig.ambient.intensity;
        }
        
        // Update directional light
        const directionalLight = this.engine.scene.children.find(child => 
            child.type === 'DirectionalLight'
        );
        if (directionalLight) {
            directionalLight.color.setHex(lightingConfig.directional.color);
            directionalLight.intensity = lightingConfig.directional.intensity;
        }
    }
    
    /**
     * Update atmosphere settings
     */
    updateAtmosphere(atmosphereConfig) {
        // Update fog
        if (this.engine.scene.fog) {
            this.engine.scene.fog.color.setHex(atmosphereConfig.fog.color);
            this.engine.scene.fog.near = atmosphereConfig.fog.near;
            this.engine.scene.fog.far = atmosphereConfig.fog.far;
        }
        
        // Update environmental data
        Object.assign(this.environmentalData, {
            humidity: atmosphereConfig.humidity,
            temperature: atmosphereConfig.temperature,
            co2: atmosphereConfig.co2
        });
    }
    
    /**
     * Update camera position for environment
     */
    updateCameraForEnvironment(environmentType) {
        if (environmentType === 'indoor') {
            gsap.to(this.engine.camera.position, {
                duration: 2,
                x: 0,
                y: 3,
                z: 6,
                ease: 'power2.inOut'
            });
        } else if (environmentType === 'outdoor') {
            gsap.to(this.engine.camera.position, {
                duration: 2,
                x: 0,
                y: 8,
                z: 12,
                ease: 'power2.inOut'
            });
        }
    }
    
    /**
     * Get current environment data
     */
    getCurrentEnvironmentData() {
        return {
            type: this.currentEnvironment,
            config: this.environmentConfigs[this.currentEnvironment],
            environmental: this.environmentalData,
            equipment: this.equipmentStatus,
            weather: this.weatherState,
            timeOfDay: this.timeOfDay
        };
    }
    
    /**
     * Get available planting positions
     */
    getAvailablePlantingPositions() {
        const positions = [];
        
        if (this.currentEnvironment === 'indoor') {
            // DWC bucket positions
            const bucketPositions = [
                { x: -1.5, y: 0.5, z: -1 },
                { x: 0, y: 0.5, z: -1 },
                { x: 1.5, y: 0.5, z: -1 },
                { x: -1.5, y: 0.5, z: 1 },
                { x: 0, y: 0.5, z: 1 },
                { x: 1.5, y: 0.5, z: 1 }
            ];
            positions.push(...bucketPositions);
        } else if (this.currentEnvironment === 'outdoor') {
            // Grid positions
            const gridSize = 5;
            const spacing = 2;
            const startX = -(gridSize - 1) * spacing / 2;
            const startZ = -(gridSize - 1) * spacing / 2;
            
            for (let x = 0; x < gridSize; x++) {
                for (let z = 0; z < gridSize; z++) {
                    positions.push({
                        x: startX + x * spacing,
                        y: 0,
                        z: startZ + z * spacing
                    });
                }
            }
        }
        
        return positions;
    }
}

