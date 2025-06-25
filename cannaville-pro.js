/**
 * CannaVille Pro - Hyper-Realistic Cannabis Growing Simulator
 * Main Game Engine with First-Person Controls and Interactive 3D Environment
 */

class CannaVillePro {
    constructor() {
        this.scene = null;
        this.camera = null;
        this.renderer = null;
        this.controls = null;
        this.clock = new THREE.Clock();
        
        // Game state
        this.gameState = {
            money: 1000,
            energy: 100,
            level: 1,
            day: 1,
            environment: 'indoor', // 'indoor' or 'outdoor'
            selectedTool: null,
            plants: [],
            environmentSettings: {
                light: 75,
                temperature: 72,
                humidity: 50
            }
        };

        // Asset management
        this.assets = {
            plants: {},
            environments: {},
            tools: {}
        };

        // Interaction system
        this.raycaster = new THREE.Raycaster();
        this.mouse = new THREE.Vector2();
        this.interactableObjects = [];

        this.init();
    }

    async init() {
        console.log('🌱 Initializing CannaVille Pro...');
        
        try {
            this.setupRenderer();
            this.setupScene();
            this.setupCamera();
            this.setupControls();
            this.setupLighting();
            await this.loadAssets();
            this.setupEnvironments();
            this.setupEventListeners();
            this.setupUI();
            this.startGameLoop();
            
            // Hide loading screen
            document.getElementById('loading-screen').style.display = 'none';
            
            console.log('✅ CannaVille Pro initialized successfully!');
        } catch (error) {
            console.error('❌ Failed to initialize CannaVille Pro:', error);
        }
    }

    setupRenderer() {
        this.renderer = new THREE.WebGLRenderer({ 
            antialias: true,
            alpha: true,
            powerPreference: "high-performance"
        });
        this.renderer.setSize(window.innerWidth, window.innerHeight);
        this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
        this.renderer.shadowMap.enabled = true;
        this.renderer.shadowMap.type = THREE.PCFSoftShadowMap;
        this.renderer.outputEncoding = THREE.sRGBEncoding;
        this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
        this.renderer.toneMappingExposure = 1.2;
        
        document.getElementById('game-container').appendChild(this.renderer.domElement);
    }

    setupScene() {
        this.scene = new THREE.Scene();
        this.scene.background = new THREE.Color(0x87CEEB); // Sky blue
        this.scene.fog = new THREE.Fog(0x87CEEB, 50, 200);
    }

    setupCamera() {
        this.camera = new THREE.PerspectiveCamera(
            75, 
            window.innerWidth / window.innerHeight, 
            0.1, 
            1000
        );
        this.camera.position.set(0, 1.7, 5); // Human eye level
    }

    setupControls() {
        this.controls = new THREE.PointerLockControls(this.camera, document.body);
        
        // Movement system
        this.moveForward = false;
        this.moveBackward = false;
        this.moveLeft = false;
        this.moveRight = false;
        this.canJump = false;
        
        this.velocity = new THREE.Vector3();
        this.direction = new THREE.Vector3();
        
        // Click to lock pointer
        document.getElementById('game-container').addEventListener('click', () => {
            this.controls.lock();
        });
    }

    setupLighting() {
        // Ambient light
        const ambientLight = new THREE.AmbientLight(0x404040, 0.4);
        this.scene.add(ambientLight);

        // Directional light (sun)
        const directionalLight = new THREE.DirectionalLight(0xffffff, 1);
        directionalLight.position.set(50, 50, 50);
        directionalLight.castShadow = true;
        directionalLight.shadow.mapSize.width = 2048;
        directionalLight.shadow.mapSize.height = 2048;
        directionalLight.shadow.camera.near = 0.5;
        directionalLight.shadow.camera.far = 500;
        directionalLight.shadow.camera.left = -50;
        directionalLight.shadow.camera.right = 50;
        directionalLight.shadow.camera.top = 50;
        directionalLight.shadow.camera.bottom = -50;
        this.scene.add(directionalLight);

        // Point lights for indoor environment
        this.indoorLights = [];
        for (let i = 0; i < 2; i++) {
            const light = new THREE.PointLight(0xff6b35, 1, 10);
            light.position.set(-2 + i * 4, 3, 0);
            light.castShadow = true;
            this.indoorLights.push(light);
            this.scene.add(light);
        }
    }

    async loadAssets() {
        console.log('📦 Loading 3D assets...');
        
        const loader = new THREE.GLTFLoader();
        const dracoLoader = new THREE.DRACOLoader();
        dracoLoader.setDecoderPath('https://www.gstatic.com/draco/v1/decoders/');
        loader.setDRACOLoader(dracoLoader);

        try {
            // Load cannabis plant models
            const plantPaths = [
                '/assets/models/plants/cannabis_plant_small_1.glb',
                '/assets/models/plants/cannabis_sativa_plant.glb',
                '/assets/models/plants/small_cannabis_plant.glb',
                '/assets/models/plants/cannabis_plant.glb'
            ];

            for (let i = 0; i < plantPaths.length; i++) {
                try {
                    const gltf = await this.loadGLTF(loader, plantPaths[i]);
                    this.assets.plants[`stage_${i}`] = gltf.scene;
                    console.log(`✅ Loaded plant stage ${i}`);
                } catch (error) {
                    console.warn(`⚠️ Could not load ${plantPaths[i]}, using fallback`);
                    this.assets.plants[`stage_${i}`] = this.createFallbackPlant(i);
                }
            }
        } catch (error) {
            console.warn('⚠️ Using fallback plant models');
            this.createFallbackPlants();
        }
    }

    loadGLTF(loader, path) {
        return new Promise((resolve, reject) => {
            loader.load(path, resolve, undefined, reject);
        });
    }

    createFallbackPlant(stage) {
        const group = new THREE.Group();
        
        // Stem
        const stemGeometry = new THREE.CylinderGeometry(0.02, 0.05, 0.5 + stage * 0.3);
        const stemMaterial = new THREE.MeshLambertMaterial({ color: 0x4a5d23 });
        const stem = new THREE.Mesh(stemGeometry, stemMaterial);
        stem.position.y = (0.5 + stage * 0.3) / 2;
        group.add(stem);

        // Leaves
        const leafCount = 3 + stage * 2;
        for (let i = 0; i < leafCount; i++) {
            const leafGeometry = new THREE.PlaneGeometry(0.1 + stage * 0.05, 0.2 + stage * 0.1);
            const leafMaterial = new THREE.MeshLambertMaterial({ 
                color: stage < 2 ? 0x228B22 : 0x32CD32,
                side: THREE.DoubleSide
            });
            const leaf = new THREE.Mesh(leafGeometry, leafMaterial);
            
            const angle = (i / leafCount) * Math.PI * 2;
            const height = 0.2 + (i / leafCount) * (0.3 + stage * 0.2);
            
            leaf.position.set(
                Math.cos(angle) * 0.1,
                height,
                Math.sin(angle) * 0.1
            );
            leaf.rotation.y = angle;
            leaf.rotation.z = Math.PI / 6;
            
            group.add(leaf);
        }

        // Buds for mature plants
        if (stage >= 2) {
            const budCount = 2 + stage;
            for (let i = 0; i < budCount; i++) {
                const budGeometry = new THREE.SphereGeometry(0.03 + stage * 0.02);
                const budMaterial = new THREE.MeshLambertMaterial({ 
                    color: stage === 3 ? 0x8B4513 : 0x9ACD32 
                });
                const bud = new THREE.Mesh(budGeometry, budMaterial);
                
                const angle = (i / budCount) * Math.PI * 2;
                bud.position.set(
                    Math.cos(angle) * 0.08,
                    0.4 + stage * 0.2,
                    Math.sin(angle) * 0.08
                );
                
                group.add(bud);
            }
        }

        group.scale.setScalar(0.5 + stage * 0.3);
        return group;
    }

    createFallbackPlants() {
        for (let i = 0; i < 4; i++) {
            this.assets.plants[`stage_${i}`] = this.createFallbackPlant(i);
        }
    }

    setupEnvironments() {
        this.createIndoorEnvironment();
        this.createOutdoorEnvironment();
        this.switchEnvironment('indoor');
    }

    createIndoorEnvironment() {
        this.indoorGroup = new THREE.Group();

        // Tent walls
        const wallMaterial = new THREE.MeshLambertMaterial({ color: 0xc0c0c0 });
        
        // Floor
        const floorGeometry = new THREE.PlaneGeometry(4, 4);
        const floorMaterial = new THREE.MeshLambertMaterial({ color: 0x333333 });
        const floor = new THREE.Mesh(floorGeometry, floorMaterial);
        floor.rotation.x = -Math.PI / 2;
        floor.receiveShadow = true;
        this.indoorGroup.add(floor);

        // Walls
        const wallGeometry = new THREE.PlaneGeometry(4, 3);
        
        // Back wall
        const backWall = new THREE.Mesh(wallGeometry, wallMaterial);
        backWall.position.set(0, 1.5, -2);
        this.indoorGroup.add(backWall);

        // Side walls
        const leftWall = new THREE.Mesh(wallGeometry, wallMaterial);
        leftWall.position.set(-2, 1.5, 0);
        leftWall.rotation.y = Math.PI / 2;
        this.indoorGroup.add(leftWall);

        const rightWall = new THREE.Mesh(wallGeometry, wallMaterial);
        rightWall.position.set(2, 1.5, 0);
        rightWall.rotation.y = -Math.PI / 2;
        this.indoorGroup.add(rightWall);

        // DWC buckets (6 buckets in 2 rows)
        this.createDWCSystem();

        this.scene.add(this.indoorGroup);
    }

    createDWCSystem() {
        const bucketPositions = [
            [-1, 0, -0.5], [0, 0, -0.5], [1, 0, -0.5],
            [-1, 0, 0.5], [0, 0, 0.5], [1, 0, 0.5]
        ];

        bucketPositions.forEach((pos, index) => {
            // Bucket
            const bucketGeometry = new THREE.CylinderGeometry(0.3, 0.25, 0.4);
            const bucketMaterial = new THREE.MeshLambertMaterial({ color: 0x222222 });
            const bucket = new THREE.Mesh(bucketGeometry, bucketMaterial);
            bucket.position.set(pos[0], 0.2, pos[2]);
            bucket.castShadow = true;
            this.indoorGroup.add(bucket);

            // Net pot
            const netPotGeometry = new THREE.CylinderGeometry(0.1, 0.08, 0.1);
            const netPotMaterial = new THREE.MeshLambertMaterial({ color: 0x8B4513 });
            const netPot = new THREE.Mesh(netPotGeometry, netPotMaterial);
            netPot.position.set(pos[0], 0.45, pos[2]);
            this.indoorGroup.add(netPot);

            // Plant (if any)
            if (index < 3) { // First 3 buckets have plants
                const plant = this.assets.plants[`stage_${index}`].clone();
                plant.position.set(pos[0], 0.5, pos[2]);
                plant.userData = { 
                    type: 'plant', 
                    stage: index, 
                    health: 85,
                    id: `plant_${index}`
                };
                this.interactableObjects.push(plant);
                this.indoorGroup.add(plant);
                this.gameState.plants.push(plant);
            }
        });

        // PVC pipes connecting buckets
        const pipeGeometry = new THREE.CylinderGeometry(0.02, 0.02, 3);
        const pipeMaterial = new THREE.MeshLambertMaterial({ color: 0xffffff });
        
        // Horizontal pipes
        const pipe1 = new THREE.Mesh(pipeGeometry, pipeMaterial);
        pipe1.position.set(0, 0.1, -0.5);
        pipe1.rotation.z = Math.PI / 2;
        this.indoorGroup.add(pipe1);

        const pipe2 = new THREE.Mesh(pipeGeometry, pipeMaterial);
        pipe2.position.set(0, 0.1, 0.5);
        pipe2.rotation.z = Math.PI / 2;
        this.indoorGroup.add(pipe2);
    }

    createOutdoorEnvironment() {
        this.outdoorGroup = new THREE.Group();

        // Ground
        const groundGeometry = new THREE.PlaneGeometry(20, 20);
        const groundMaterial = new THREE.MeshLambertMaterial({ color: 0x4a5d23 });
        const ground = new THREE.Mesh(groundGeometry, groundMaterial);
        ground.rotation.x = -Math.PI / 2;
        ground.receiveShadow = true;
        this.outdoorGroup.add(ground);

        // 5x5 grid of holes
        for (let x = 0; x < 5; x++) {
            for (let z = 0; z < 5; z++) {
                const holeGeometry = new THREE.CylinderGeometry(0.3, 0.25, 0.3);
                const holeMaterial = new THREE.MeshLambertMaterial({ color: 0x2d1810 });
                const hole = new THREE.Mesh(holeGeometry, holeMaterial);
                hole.position.set((x - 2) * 1.2, -0.1, (z - 2) * 1.2);
                this.outdoorGroup.add(hole);

                // Add plants to some holes
                if (Math.random() > 0.6) {
                    const plantStage = Math.floor(Math.random() * 4);
                    const plant = this.assets.plants[`stage_${plantStage}`].clone();
                    plant.position.set((x - 2) * 1.2, 0.1, (z - 2) * 1.2);
                    plant.userData = { 
                        type: 'plant', 
                        stage: plantStage, 
                        health: 70 + Math.random() * 30,
                        id: `outdoor_plant_${x}_${z}`
                    };
                    this.interactableObjects.push(plant);
                    this.outdoorGroup.add(plant);
                    this.gameState.plants.push(plant);
                }
            }
        }

        this.scene.add(this.outdoorGroup);
    }

    switchEnvironment(env) {
        this.gameState.environment = env;
        
        if (env === 'indoor') {
            this.indoorGroup.visible = true;
            this.outdoorGroup.visible = false;
            this.camera.position.set(0, 1.7, 3);
            this.scene.fog.far = 10;
            
            // Enable indoor lights
            this.indoorLights.forEach(light => light.visible = true);
        } else {
            this.indoorGroup.visible = false;
            this.outdoorGroup.visible = true;
            this.camera.position.set(0, 1.7, 8);
            this.scene.fog.far = 200;
            
            // Disable indoor lights
            this.indoorLights.forEach(light => light.visible = false);
        }

        // Update UI
        document.querySelectorAll('.env-button').forEach(btn => btn.classList.remove('active'));
        document.getElementById(env === 'indoor' ? 'indoor-btn' : 'outdoor-btn').classList.add('active');
    }

    setupEventListeners() {
        // Keyboard controls
        document.addEventListener('keydown', (event) => {
            switch (event.code) {
                case 'ArrowUp':
                case 'KeyW':
                    this.moveForward = true;
                    break;
                case 'ArrowLeft':
                case 'KeyA':
                    this.moveLeft = true;
                    break;
                case 'ArrowDown':
                case 'KeyS':
                    this.moveBackward = true;
                    break;
                case 'ArrowRight':
                case 'KeyD':
                    this.moveRight = true;
                    break;
                case 'Space':
                    if (this.canJump === true) this.velocity.y += 350;
                    this.canJump = false;
                    break;
                case 'KeyE':
                    this.interactWithNearestObject();
                    break;
            }
        });

        document.addEventListener('keyup', (event) => {
            switch (event.code) {
                case 'ArrowUp':
                case 'KeyW':
                    this.moveForward = false;
                    break;
                case 'ArrowLeft':
                case 'KeyA':
                    this.moveLeft = false;
                    break;
                case 'ArrowDown':
                case 'KeyS':
                    this.moveBackward = false;
                    break;
                case 'ArrowRight':
                case 'KeyD':
                    this.moveRight = false;
                    break;
            }
        });

        // Mouse interaction
        document.addEventListener('click', (event) => {
            if (this.controls.isLocked) {
                this.handleClick(event);
            }
        });

        // Window resize
        window.addEventListener('resize', () => {
            this.camera.aspect = window.innerWidth / window.innerHeight;
            this.camera.updateProjectionMatrix();
            this.renderer.setSize(window.innerWidth, window.innerHeight);
        });
    }

    setupUI() {
        // Environment buttons
        document.getElementById('indoor-btn').addEventListener('click', () => {
            this.switchEnvironment('indoor');
        });

        document.getElementById('outdoor-btn').addEventListener('click', () => {
            this.switchEnvironment('outdoor');
        });

        // Tool buttons
        document.querySelectorAll('.tool-button').forEach(button => {
            button.addEventListener('click', () => {
                // Remove active class from all buttons
                document.querySelectorAll('.tool-button').forEach(btn => btn.classList.remove('active'));
                
                // Add active class to clicked button
                button.classList.add('active');
                
                // Set selected tool
                this.gameState.selectedTool = button.id;
                console.log('Selected tool:', this.gameState.selectedTool);
            });
        });

        // Sliders
        document.getElementById('light-slider').addEventListener('input', (e) => {
            this.gameState.environmentSettings.light = e.target.value;
            document.getElementById('light-value').textContent = e.target.value + '%';
            this.updateEnvironmentLighting();
        });

        document.getElementById('temp-slider').addEventListener('input', (e) => {
            this.gameState.environmentSettings.temperature = e.target.value;
            document.getElementById('temp-value').textContent = e.target.value + '°F';
        });

        document.getElementById('humidity-slider').addEventListener('input', (e) => {
            this.gameState.environmentSettings.humidity = e.target.value;
            document.getElementById('humidity-value').textContent = e.target.value + '%';
        });

        // Update UI periodically
        setInterval(() => {
            this.updateUI();
        }, 1000);
    }

    updateEnvironmentLighting() {
        const intensity = this.gameState.environmentSettings.light / 100;
        this.indoorLights.forEach(light => {
            light.intensity = intensity;
        });
    }

    handleClick(event) {
        // Raycast to find clicked object
        this.raycaster.setFromCamera(new THREE.Vector2(0, 0), this.camera);
        const intersects = this.raycaster.intersectObjects(this.interactableObjects, true);

        if (intersects.length > 0) {
            const object = intersects[0].object.parent || intersects[0].object;
            this.interactWithObject(object);
        }
    }

    interactWithNearestObject() {
        // Find nearest interactable object within range
        const playerPosition = this.camera.position;
        let nearestObject = null;
        let nearestDistance = Infinity;

        this.interactableObjects.forEach(obj => {
            const distance = playerPosition.distanceTo(obj.position);
            if (distance < 2 && distance < nearestDistance) {
                nearestDistance = distance;
                nearestObject = obj;
            }
        });

        if (nearestObject) {
            this.interactWithObject(nearestObject);
        }
    }

    interactWithObject(object) {
        if (object.userData.type === 'plant') {
            console.log('Interacting with plant:', object.userData.id);
            
            switch (this.gameState.selectedTool) {
                case 'water-tool':
                    this.waterPlant(object);
                    break;
                case 'neem-tool':
                    this.treatPlant(object, 'neem');
                    break;
                case 'nutrient-tool':
                    this.feedPlant(object);
                    break;
                case 'inspect-btn':
                    this.inspectPlant(object);
                    break;
                case 'harvest-btn':
                    this.harvestPlant(object);
                    break;
                default:
                    this.inspectPlant(object);
            }
        }
    }

    waterPlant(plant) {
        plant.userData.health = Math.min(100, plant.userData.health + 5);
        console.log(`💧 Watered plant ${plant.userData.id}. Health: ${plant.userData.health}%`);
        this.showFloatingText(plant.position, '+5 Health', 0x00ff00);
    }

    treatPlant(plant, treatment) {
        plant.userData.health = Math.min(100, plant.userData.health + 10);
        console.log(`🦗 Treated plant ${plant.userData.id} with ${treatment}. Health: ${plant.userData.health}%`);
        this.showFloatingText(plant.position, '+10 Health', 0x00ff00);
    }

    feedPlant(plant) {
        plant.userData.health = Math.min(100, plant.userData.health + 15);
        console.log(`🧪 Fed plant ${plant.userData.id} nutrients. Health: ${plant.userData.health}%`);
        this.showFloatingText(plant.position, '+15 Health', 0x00ff00);
    }

    inspectPlant(plant) {
        console.log(`🔍 Inspecting plant ${plant.userData.id}:`);
        console.log(`- Stage: ${plant.userData.stage}`);
        console.log(`- Health: ${plant.userData.health}%`);
        
        // Update UI with plant info
        document.querySelector('.health-fill').style.width = plant.userData.health + '%';
    }

    harvestPlant(plant) {
        if (plant.userData.stage >= 3) {
            const reward = 50 + Math.floor(Math.random() * 50);
            this.gameState.money += reward;
            console.log(`✂️ Harvested plant ${plant.userData.id} for $${reward}`);
            this.showFloatingText(plant.position, `+$${reward}`, 0xffd700);
            
            // Remove plant from scene
            plant.parent.remove(plant);
            const index = this.interactableObjects.indexOf(plant);
            if (index > -1) {
                this.interactableObjects.splice(index, 1);
            }
        } else {
            console.log('🚫 Plant not ready for harvest');
            this.showFloatingText(plant.position, 'Not Ready', 0xff0000);
        }
    }

    showFloatingText(position, text, color) {
        // Create floating text effect (simplified)
        console.log(`💬 ${text} at position:`, position);
    }

    updateMovement(delta) {
        if (!this.controls.isLocked) return;

        this.velocity.x -= this.velocity.x * 10.0 * delta;
        this.velocity.z -= this.velocity.z * 10.0 * delta;
        this.velocity.y -= 9.8 * 100.0 * delta; // Gravity

        this.direction.z = Number(this.moveForward) - Number(this.moveBackward);
        this.direction.x = Number(this.moveRight) - Number(this.moveLeft);
        this.direction.normalize();

        if (this.moveForward || this.moveBackward) this.velocity.z -= this.direction.z * 400.0 * delta;
        if (this.moveLeft || this.moveRight) this.velocity.x -= this.direction.x * 400.0 * delta;

        this.controls.moveRight(-this.velocity.x * delta);
        this.controls.moveForward(-this.velocity.z * delta);

        this.controls.getObject().position.y += (this.velocity.y * delta);

        if (this.controls.getObject().position.y < 1.7) {
            this.velocity.y = 0;
            this.controls.getObject().position.y = 1.7;
            this.canJump = true;
        }
    }

    updateUI() {
        document.getElementById('money').textContent = `$${this.gameState.money}`;
        document.getElementById('energy').textContent = this.gameState.energy;
        document.getElementById('level').textContent = this.gameState.level;
        document.getElementById('day').textContent = this.gameState.day;
    }

    startGameLoop() {
        const animate = () => {
            requestAnimationFrame(animate);
            
            const delta = this.clock.getDelta();
            
            this.updateMovement(delta);
            
            // Animate plants (gentle swaying)
            this.gameState.plants.forEach((plant, index) => {
                if (plant.parent) {
                    const time = this.clock.getElapsedTime();
                    plant.rotation.z = Math.sin(time + index) * 0.02;
                }
            });
            
            this.renderer.render(this.scene, this.camera);
        };
        
        animate();
    }
}

// Initialize the game when the page loads
document.addEventListener('DOMContentLoaded', () => {
    window.cannaVillePro = new CannaVillePro();
});

