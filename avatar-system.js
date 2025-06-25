/**
 * CannaVille Pro - Avatar System
 * Hyper-realistic avatar management with diverse character options
 */

class AvatarSystem {
    constructor(game) {
        this.game = game;
        this.currentAvatar = null;
        this.avatarModels = {};
        this.animations = {};
        this.isLoaded = false;
        
        // Avatar configurations
        this.avatarConfigs = {
            male_caucasian: {
                name: 'Jake Thompson',
                description: 'Experienced outdoor grower from Colorado',
                skinTone: 0xffdbac,
                hairColor: 0x8b4513,
                eyeColor: 0x4169e1,
                clothing: 'farmer_overalls',
                personality: 'methodical',
                specialties: ['outdoor_growing', 'soil_management']
            },
            female_caucasian: {
                name: 'Sarah Mitchell',
                description: 'Hydroponic specialist and botanist',
                skinTone: 0xffdbac,
                hairColor: 0xdaa520,
                eyeColor: 0x228b22,
                clothing: 'lab_coat',
                personality: 'scientific',
                specialties: ['hydroponics', 'plant_genetics']
            },
            male_african: {
                name: 'Marcus Johnson',
                description: 'Organic cultivation expert from California',
                skinTone: 0x8b4513,
                hairColor: 0x2f1b14,
                eyeColor: 0x654321,
                clothing: 'casual_farmer',
                personality: 'innovative',
                specialties: ['organic_methods', 'pest_control']
            },
            female_african: {
                name: 'Amara Williams',
                description: 'Sustainable farming advocate and educator',
                skinTone: 0x8b4513,
                hairColor: 0x2f1b14,
                eyeColor: 0x654321,
                clothing: 'eco_friendly',
                personality: 'nurturing',
                specialties: ['sustainability', 'education']
            },
            male_asian: {
                name: 'Hiroshi Tanaka',
                description: 'Precision agriculture technologist',
                skinTone: 0xffd39a,
                hairColor: 0x2f1b14,
                eyeColor: 0x654321,
                clothing: 'tech_wear',
                personality: 'precise',
                specialties: ['technology', 'automation']
            },
            female_asian: {
                name: 'Li Wei Chen',
                description: 'Traditional medicine and cannabis researcher',
                skinTone: 0xffd39a,
                hairColor: 0x2f1b14,
                eyeColor: 0x654321,
                clothing: 'research_attire',
                personality: 'analytical',
                specialties: ['research', 'medicinal_cannabis']
            },
            male_hispanic: {
                name: 'Carlos Rodriguez',
                description: 'Multi-generational farmer with family traditions',
                skinTone: 0xdeb887,
                hairColor: 0x2f1b14,
                eyeColor: 0x654321,
                clothing: 'traditional_farmer',
                personality: 'traditional',
                specialties: ['heritage_strains', 'family_methods']
            },
            female_hispanic: {
                name: 'Isabella Martinez',
                description: 'Cannabis entrepreneur and business owner',
                skinTone: 0xdeb887,
                hairColor: 0x2f1b14,
                eyeColor: 0x654321,
                clothing: 'business_casual',
                personality: 'entrepreneurial',
                specialties: ['business', 'marketing']
            }
        };
        
        // Animation states
        this.animationStates = {
            idle: { duration: 3000, loop: true },
            walking: { duration: 1000, loop: true },
            planting: { duration: 2000, loop: false },
            watering: { duration: 1500, loop: false },
            harvesting: { duration: 2500, loop: false },
            examining: { duration: 2000, loop: false },
            celebrating: { duration: 3000, loop: false }
        };
        
        this.currentAnimation = 'idle';
        this.animationMixer = null;
        
        this.init();
    }
    
    async init() {
        console.log('👤 Initializing Avatar System...');
        
        try {
            // Load avatar models
            await this.loadAvatarModels();
            
            // Setup animation system
            this.setupAnimationSystem();
            
            // Setup avatar selection UI
            this.setupAvatarSelectionUI();
            
            this.isLoaded = true;
            console.log('✅ Avatar System initialized successfully');
            
        } catch (error) {
            console.error('❌ Failed to initialize Avatar System:', error);
        }
    }
    
    async loadAvatarModels() {
        console.log('📦 Loading avatar models...');
        
        // For demo purposes, create enhanced placeholder models
        // In production, these would load actual GLTF models with rigging
        for (const [avatarId, config] of Object.entries(this.avatarConfigs)) {
            this.avatarModels[avatarId] = this.createEnhancedAvatarModel(config);
        }
    }
    
    createEnhancedAvatarModel(config) {
        const group = new THREE.Group();
        group.name = config.name;
        
        // Create more detailed avatar model
        const avatar = this.createDetailedAvatar(config);
        group.add(avatar);
        
        // Add avatar accessories
        const accessories = this.createAvatarAccessories(config);
        accessories.forEach(accessory => group.add(accessory));
        
        // Add name tag
        const nameTag = this.createNameTag(config.name);
        nameTag.position.y = 2.5;
        group.add(nameTag);
        
        return group;
    }
    
    createDetailedAvatar(config) {
        const avatar = new THREE.Group();
        
        // Body with more realistic proportions
        const bodyGeometry = new THREE.CapsuleGeometry(0.25, 1.2, 8, 16);
        const bodyMaterial = new THREE.MeshLambertMaterial({ 
            color: this.getClothingColor(config.clothing),
            transparent: true,
            opacity: 0.9
        });
        const body = new THREE.Mesh(bodyGeometry, bodyMaterial);
        body.position.y = 1.0;
        body.castShadow = true;
        avatar.add(body);
        
        // Head with realistic skin tone
        const headGeometry = new THREE.SphereGeometry(0.22, 16, 16);
        const headMaterial = new THREE.MeshLambertMaterial({ 
            color: config.skinTone,
            transparent: true,
            opacity: 0.95
        });
        const head = new THREE.Mesh(headGeometry, headMaterial);
        head.position.y = 1.8;
        head.castShadow = true;
        avatar.add(head);
        
        // Hair
        const hair = this.createHair(config);
        hair.position.y = 1.8;
        avatar.add(hair);
        
        // Eyes
        const eyes = this.createEyes(config);
        eyes.position.set(0, 1.8, 0.15);
        avatar.add(eyes);
        
        // Arms with realistic positioning
        const armGeometry = new THREE.CapsuleGeometry(0.08, 1.0, 6, 12);
        const armMaterial = new THREE.MeshLambertMaterial({ color: config.skinTone });
        
        const leftArm = new THREE.Mesh(armGeometry, armMaterial);
        leftArm.position.set(-0.4, 1.2, 0);
        leftArm.rotation.z = Math.PI / 8;
        leftArm.castShadow = true;
        avatar.add(leftArm);
        
        const rightArm = new THREE.Mesh(armGeometry, armMaterial);
        rightArm.position.set(0.4, 1.2, 0);
        rightArm.rotation.z = -Math.PI / 8;
        rightArm.castShadow = true;
        avatar.add(rightArm);
        
        // Legs with realistic proportions
        const legGeometry = new THREE.CapsuleGeometry(0.1, 1.3, 6, 12);
        const legMaterial = new THREE.MeshLambertMaterial({ 
            color: this.getPantsColor(config.clothing)
        });
        
        const leftLeg = new THREE.Mesh(legGeometry, legMaterial);
        leftLeg.position.set(-0.15, 0.35, 0);
        leftLeg.castShadow = true;
        avatar.add(leftLeg);
        
        const rightLeg = new THREE.Mesh(legGeometry, legMaterial);
        rightLeg.position.set(0.15, 0.35, 0);
        rightLeg.castShadow = true;
        avatar.add(rightLeg);
        
        // Hands
        const handGeometry = new THREE.SphereGeometry(0.06, 8, 8);
        const handMaterial = new THREE.MeshLambertMaterial({ color: config.skinTone });
        
        const leftHand = new THREE.Mesh(handGeometry, handMaterial);
        leftHand.position.set(-0.5, 0.7, 0);
        leftHand.castShadow = true;
        avatar.add(leftHand);
        
        const rightHand = new THREE.Mesh(handGeometry, handMaterial);
        rightHand.position.set(0.5, 0.7, 0);
        rightHand.castShadow = true;
        avatar.add(rightHand);
        
        // Feet
        const footGeometry = new THREE.BoxGeometry(0.15, 0.08, 0.25);
        const footMaterial = new THREE.MeshLambertMaterial({ color: 0x654321 });
        
        const leftFoot = new THREE.Mesh(footGeometry, footMaterial);
        leftFoot.position.set(-0.15, -0.3, 0.05);
        leftFoot.castShadow = true;
        avatar.add(leftFoot);
        
        const rightFoot = new THREE.Mesh(footGeometry, footMaterial);
        rightFoot.position.set(0.15, -0.3, 0.05);
        rightFoot.castShadow = true;
        avatar.add(rightFoot);
        
        return avatar;
    }
    
    createHair(config) {
        const hairGroup = new THREE.Group();
        
        // Main hair volume
        const hairGeometry = new THREE.SphereGeometry(0.25, 12, 12);
        const hairMaterial = new THREE.MeshLambertMaterial({ 
            color: config.hairColor,
            transparent: true,
            opacity: 0.8
        });
        const hair = new THREE.Mesh(hairGeometry, hairMaterial);
        hair.scale.set(1, 0.8, 1);
        hair.position.y = 0.1;
        hair.castShadow = true;
        hairGroup.add(hair);
        
        // Hair details based on avatar type
        if (config.name.includes('Sarah') || config.name.includes('Amara')) {
            // Longer hair for female avatars
            const hairBack = new THREE.Mesh(hairGeometry, hairMaterial);
            hairBack.scale.set(0.8, 1.2, 0.6);
            hairBack.position.set(0, -0.1, -0.15);
            hairBack.castShadow = true;
            hairGroup.add(hairBack);
        }
        
        return hairGroup;
    }
    
    createEyes(config) {
        const eyeGroup = new THREE.Group();
        
        // Eye whites
        const eyeWhiteGeometry = new THREE.SphereGeometry(0.03, 8, 8);
        const eyeWhiteMaterial = new THREE.MeshLambertMaterial({ color: 0xffffff });
        
        const leftEyeWhite = new THREE.Mesh(eyeWhiteGeometry, eyeWhiteMaterial);
        leftEyeWhite.position.set(-0.08, 0.02, 0);
        eyeGroup.add(leftEyeWhite);
        
        const rightEyeWhite = new THREE.Mesh(eyeWhiteGeometry, eyeWhiteMaterial);
        rightEyeWhite.position.set(0.08, 0.02, 0);
        eyeGroup.add(rightEyeWhite);
        
        // Pupils
        const pupilGeometry = new THREE.SphereGeometry(0.015, 6, 6);
        const pupilMaterial = new THREE.MeshLambertMaterial({ color: config.eyeColor });
        
        const leftPupil = new THREE.Mesh(pupilGeometry, pupilMaterial);
        leftPupil.position.set(-0.08, 0.02, 0.02);
        eyeGroup.add(leftPupil);
        
        const rightPupil = new THREE.Mesh(pupilGeometry, pupilMaterial);
        rightPupil.position.set(0.08, 0.02, 0.02);
        eyeGroup.add(rightPupil);
        
        return eyeGroup;
    }
    
    createAvatarAccessories(config) {
        const accessories = [];
        
        // Add accessories based on avatar type
        switch (config.clothing) {
            case 'lab_coat':
                accessories.push(this.createLabCoat());
                break;
            case 'farmer_overalls':
                accessories.push(this.createOveralls());
                break;
            case 'tech_wear':
                accessories.push(this.createTechAccessories());
                break;
        }
        
        return accessories;
    }
    
    createLabCoat() {
        const coatGeometry = new THREE.BoxGeometry(0.6, 1.4, 0.3);
        const coatMaterial = new THREE.MeshLambertMaterial({ 
            color: 0xffffff,
            transparent: true,
            opacity: 0.9
        });
        const coat = new THREE.Mesh(coatGeometry, coatMaterial);
        coat.position.y = 1.0;
        coat.castShadow = true;
        return coat;
    }
    
    createOveralls() {
        const overallsGeometry = new THREE.BoxGeometry(0.5, 1.2, 0.25);
        const overallsMaterial = new THREE.MeshLambertMaterial({ color: 0x4169e1 });
        const overalls = new THREE.Mesh(overallsGeometry, overallsMaterial);
        overalls.position.y = 1.0;
        overalls.castShadow = true;
        return overalls;
    }
    
    createTechAccessories() {
        const group = new THREE.Group();
        
        // Tablet
        const tabletGeometry = new THREE.BoxGeometry(0.15, 0.2, 0.02);
        const tabletMaterial = new THREE.MeshLambertMaterial({ color: 0x333333 });
        const tablet = new THREE.Mesh(tabletGeometry, tabletMaterial);
        tablet.position.set(0.3, 0.8, 0);
        tablet.castShadow = true;
        group.add(tablet);
        
        return group;
    }
    
    createNameTag(name) {
        // Create a simple text sprite for the name
        const canvas = document.createElement('canvas');
        const context = canvas.getContext('2d');
        canvas.width = 256;
        canvas.height = 64;
        
        context.fillStyle = 'rgba(0, 0, 0, 0.8)';
        context.fillRect(0, 0, canvas.width, canvas.height);
        
        context.fillStyle = 'white';
        context.font = '20px Arial';
        context.textAlign = 'center';
        context.fillText(name, canvas.width / 2, canvas.height / 2 + 7);
        
        const texture = new THREE.CanvasTexture(canvas);
        const material = new THREE.SpriteMaterial({ map: texture });
        const sprite = new THREE.Sprite(material);
        sprite.scale.set(1, 0.25, 1);
        
        return sprite;
    }
    
    getClothingColor(clothing) {
        const colors = {
            farmer_overalls: 0x4169e1,
            lab_coat: 0xffffff,
            casual_farmer: 0x228b22,
            eco_friendly: 0x90ee90,
            tech_wear: 0x333333,
            research_attire: 0x800080,
            traditional_farmer: 0x8b4513,
            business_casual: 0x2f4f4f
        };
        return colors[clothing] || 0x4169e1;
    }
    
    getPantsColor(clothing) {
        const colors = {
            farmer_overalls: 0x4169e1,
            lab_coat: 0x333333,
            casual_farmer: 0x654321,
            eco_friendly: 0x8b4513,
            tech_wear: 0x000000,
            research_attire: 0x333333,
            traditional_farmer: 0x654321,
            business_casual: 0x2f4f4f
        };
        return colors[clothing] || 0x333333;
    }
    
    setupAnimationSystem() {
        // Create animation mixer for avatar animations
        this.animationMixer = new THREE.AnimationMixer();
        
        // Create basic animations
        this.createBasicAnimations();
    }
    
    createBasicAnimations() {
        // Create simple procedural animations
        this.animations = {
            idle: this.createIdleAnimation(),
            walking: this.createWalkingAnimation(),
            planting: this.createPlantingAnimation(),
            watering: this.createWateringAnimation(),
            harvesting: this.createHarvestingAnimation()
        };
    }
    
    createIdleAnimation() {
        return {
            update: (avatar, time) => {
                if (avatar) {
                    // Subtle breathing animation
                    avatar.scale.y = 1 + Math.sin(time * 2) * 0.02;
                    
                    // Slight head movement
                    const head = avatar.children.find(child => 
                        child.children && child.children.length > 1
                    );
                    if (head) {
                        head.rotation.y = Math.sin(time * 0.5) * 0.1;
                    }
                }
            }
        };
    }
    
    createWalkingAnimation() {
        return {
            update: (avatar, time) => {
                if (avatar) {
                    // Walking bob
                    avatar.position.y = Math.sin(time * 8) * 0.05;
                    
                    // Arm swing
                    const arms = avatar.children.filter(child => 
                        child.geometry && child.geometry.type === 'CapsuleGeometry'
                    );
                    arms.forEach((arm, index) => {
                        if (index < 2) { // First two are arms
                            arm.rotation.x = Math.sin(time * 8 + index * Math.PI) * 0.3;
                        }
                    });
                }
            }
        };
    }
    
    createPlantingAnimation() {
        return {
            update: (avatar, time) => {
                if (avatar) {
                    // Bending down motion
                    avatar.rotation.x = Math.sin(time * 3) * 0.3;
                    
                    // Arm movement
                    const arms = avatar.children.filter(child => 
                        child.geometry && child.geometry.type === 'CapsuleGeometry'
                    );
                    if (arms.length >= 2) {
                        arms[0].rotation.x = -Math.PI / 3; // Left arm down
                        arms[1].rotation.x = -Math.PI / 4; // Right arm down
                    }
                }
            }
        };
    }
    
    createWateringAnimation() {
        return {
            update: (avatar, time) => {
                if (avatar) {
                    // Watering can motion
                    const rightArm = avatar.children.find(child => 
                        child.position.x > 0 && child.geometry && 
                        child.geometry.type === 'CapsuleGeometry'
                    );
                    if (rightArm) {
                        rightArm.rotation.x = Math.sin(time * 4) * 0.2 - 0.3;
                        rightArm.rotation.z = -Math.PI / 6;
                    }
                }
            }
        };
    }
    
    createHarvestingAnimation() {
        return {
            update: (avatar, time) => {
                if (avatar) {
                    // Cutting motion
                    const arms = avatar.children.filter(child => 
                        child.geometry && child.geometry.type === 'CapsuleGeometry'
                    );
                    if (arms.length >= 2) {
                        arms.forEach(arm => {
                            arm.rotation.x = Math.sin(time * 6) * 0.4 - 0.2;
                        });
                    }
                }
            }
        };
    }
    
    setupAvatarSelectionUI() {
        const avatarOptions = document.querySelectorAll('.avatar-option');
        const confirmButton = document.getElementById('confirm-avatar');
        
        avatarOptions.forEach(option => {
            option.addEventListener('click', () => {
                // Remove selection from all options
                avatarOptions.forEach(opt => opt.classList.remove('selected'));
                
                // Add selection to clicked option
                option.classList.add('selected');
                
                // Store selected avatar
                const avatarId = option.dataset.avatar;
                this.selectedAvatarId = avatarId;
                
                // Update character info
                this.updateCharacterInfo(avatarId);
                
                // Enable confirm button
                confirmButton.disabled = false;
            });
        });
        
        confirmButton.addEventListener('click', () => {
            if (this.selectedAvatarId) {
                this.selectAvatar(this.selectedAvatarId);
                this.hideAvatarSelection();
            }
        });
    }
    
    updateCharacterInfo(avatarId) {
        const config = this.avatarConfigs[avatarId];
        if (config) {
            // Update character name display
            const characterNameElement = document.getElementById('characterName');
            if (characterNameElement) {
                characterNameElement.textContent = config.name;
            }
            
            // You could add more character info display here
            console.log(`Selected: ${config.name} - ${config.description}`);
        }
    }
    
    selectAvatar(avatarId) {
        console.log(`🎭 Selecting avatar: ${avatarId}`);
        
        // Remove current avatar if exists
        if (this.currentAvatar) {
            this.game.scene.remove(this.currentAvatar);
        }
        
        // Add new avatar to scene
        this.currentAvatar = this.avatarModels[avatarId].clone();
        this.currentAvatar.position.set(0, 0, 3);
        this.game.scene.add(this.currentAvatar);
        
        // Update game state
        this.game.gameState.selectedAvatar = avatarId;
        this.game.avatarModel = this.currentAvatar;
        
        // Start idle animation
        this.playAnimation('idle');
        
        // Show success notification
        const config = this.avatarConfigs[avatarId];
        this.game.notificationManager.show(
            'Avatar Selected! 👤',
            `Welcome ${config.name}! Ready to start growing?`,
            'success'
        );
        
        console.log(`✅ Avatar ${config.name} selected successfully`);
    }
    
    playAnimation(animationName, duration = null) {
        if (this.animations[animationName]) {
            this.currentAnimation = animationName;
            
            if (duration) {
                setTimeout(() => {
                    this.playAnimation('idle');
                }, duration);
            }
            
            console.log(`🎬 Playing animation: ${animationName}`);
        }
    }
    
    update(deltaTime) {
        if (this.currentAvatar && this.animations[this.currentAnimation]) {
            const time = Date.now() * 0.001;
            this.animations[this.currentAnimation].update(this.currentAvatar, time);
        }
    }
    
    hideAvatarSelection() {
        const avatarPanel = document.getElementById('avatar-panel');
        if (avatarPanel) {
            avatarPanel.style.display = 'none';
        }
    }
    
    showAvatarSelection() {
        const avatarPanel = document.getElementById('avatar-panel');
        if (avatarPanel) {
            avatarPanel.style.display = 'block';
        }
    }
    
    getAvatarInfo(avatarId) {
        return this.avatarConfigs[avatarId] || null;
    }
    
    getCurrentAvatarInfo() {
        if (this.game.gameState.selectedAvatar) {
            return this.getAvatarInfo(this.game.gameState.selectedAvatar);
        }
        return null;
    }
    
    // Avatar movement methods
    moveAvatarTo(position, callback = null) {
        if (this.currentAvatar) {
            this.playAnimation('walking');
            
            // Simple movement animation using GSAP if available, otherwise basic interpolation
            if (window.gsap) {
                gsap.to(this.currentAvatar.position, {
                    duration: 2,
                    x: position.x,
                    z: position.z,
                    ease: "power2.inOut",
                    onComplete: () => {
                        this.playAnimation('idle');
                        if (callback) callback();
                    }
                });
            } else {
                // Basic interpolation fallback
                const startPos = this.currentAvatar.position.clone();
                const targetPos = new THREE.Vector3(position.x, startPos.y, position.z);
                let progress = 0;
                
                const animate = () => {
                    progress += 0.02;
                    if (progress >= 1) {
                        this.currentAvatar.position.copy(targetPos);
                        this.playAnimation('idle');
                        if (callback) callback();
                        return;
                    }
                    
                    this.currentAvatar.position.lerpVectors(startPos, targetPos, progress);
                    requestAnimationFrame(animate);
                };
                
                animate();
            }
        }
    }
    
    performAction(action, target = null) {
        if (!this.currentAvatar) return;
        
        switch (action) {
            case 'plant':
                this.playAnimation('planting', 2000);
                break;
            case 'water':
                this.playAnimation('watering', 1500);
                break;
            case 'harvest':
                this.playAnimation('harvesting', 2500);
                break;
            case 'examine':
                this.playAnimation('examining', 2000);
                break;
            case 'celebrate':
                this.playAnimation('celebrating', 3000);
                break;
        }
        
        // Move to target if specified
        if (target && target.position) {
            const targetPos = {
                x: target.position.x,
                z: target.position.z
            };
            this.moveAvatarTo(targetPos);
        }
    }
}

// Export for use in main game
window.AvatarSystem = AvatarSystem;

