/**
 * CannaVille Pro - Input System
 * Handles user input for both desktop and mobile devices
 */

export class InputSystem {
    constructor(engine) {
        this.engine = engine;
        this.isInitialized = false;
        
        // Input state
        this.keys = {};
        this.mouse = {
            x: 0,
            y: 0,
            isDown: false,
            button: 0
        };
        
        // Touch state
        this.touches = new Map();
        this.joystickActive = false;
        this.joystickCenter = { x: 0, y: 0 };
        this.joystickPosition = { x: 0, y: 0 };
        
        // Movement state
        this.movement = {
            forward: false,
            backward: false,
            left: false,
            right: false,
            up: false,
            down: false
        };
        
        // Camera controls
        this.cameraRotation = { x: 0, y: 0 };
        this.sensitivity = 0.002;
        this.isPointerLocked = false;
    }
    
    /**
     * Initialize the input system
     */
    async init() {
        console.log('🎮 Initializing Input System...');
        
        try {
            this.setupKeyboardControls();
            this.setupMouseControls();
            this.setupTouchControls();
            
            if (this.engine.deviceInfo.isMobile) {
                this.setupMobileControls();
            }
            
            this.isInitialized = true;
            console.log('✅ Input System initialized');
            
        } catch (error) {
            console.error('❌ Failed to initialize Input System:', error);
            throw error;
        }
    }
    
    /**
     * Setup keyboard controls
     */
    setupKeyboardControls() {
        document.addEventListener('keydown', (event) => {
            this.keys[event.code] = true;
            this.updateMovementFromKeys();
            
            // Prevent default for game keys
            if (this.isGameKey(event.code)) {
                event.preventDefault();
            }
        });
        
        document.addEventListener('keyup', (event) => {
            this.keys[event.code] = false;
            this.updateMovementFromKeys();
        });
    }
    
    /**
     * Setup mouse controls
     */
    setupMouseControls() {
        const canvas = this.engine.renderer.domElement;
        
        // Mouse movement
        document.addEventListener('mousemove', (event) => {
            this.mouse.x = event.clientX;
            this.mouse.y = event.clientY;
            
            if (this.isPointerLocked) {
                this.updateCameraFromMouse(event.movementX, event.movementY);
            }
        });
        
        // Mouse buttons
        canvas.addEventListener('mousedown', (event) => {
            this.mouse.isDown = true;
            this.mouse.button = event.button;
            
            // Request pointer lock on left click
            if (event.button === 0) {
                this.requestPointerLock();
            }
        });
        
        canvas.addEventListener('mouseup', (event) => {
            this.mouse.isDown = false;
        });
        
        // Pointer lock events
        document.addEventListener('pointerlockchange', () => {
            this.isPointerLocked = document.pointerLockElement === canvas;
        });
        
        // Wheel for zoom
        canvas.addEventListener('wheel', (event) => {
            event.preventDefault();
            this.handleZoom(event.deltaY);
        });
    }
    
    /**
     * Setup touch controls
     */
    setupTouchControls() {
        const canvas = this.engine.renderer.domElement;
        
        canvas.addEventListener('touchstart', (event) => {
            event.preventDefault();
            
            for (let touch of event.changedTouches) {
                this.touches.set(touch.identifier, {
                    x: touch.clientX,
                    y: touch.clientY,
                    startX: touch.clientX,
                    startY: touch.clientY
                });
            }
        });
        
        canvas.addEventListener('touchmove', (event) => {
            event.preventDefault();
            
            for (let touch of event.changedTouches) {
                const touchData = this.touches.get(touch.identifier);
                if (touchData) {
                    const deltaX = touch.clientX - touchData.x;
                    const deltaY = touch.clientY - touchData.y;
                    
                    // Camera rotation for first touch
                    if (touch.identifier === 0) {
                        this.updateCameraFromMouse(deltaX, deltaY);
                    }
                    
                    touchData.x = touch.clientX;
                    touchData.y = touch.clientY;
                }
            }
        });
        
        canvas.addEventListener('touchend', (event) => {
            event.preventDefault();
            
            for (let touch of event.changedTouches) {
                this.touches.delete(touch.identifier);
            }
        });
    }
    
    /**
     * Setup mobile-specific controls
     */
    setupMobileControls() {
        this.createVirtualJoystick();
    }
    
    /**
     * Create virtual joystick for mobile
     */
    createVirtualJoystick() {
        const joystick = document.getElementById('virtual-joystick');
        if (!joystick) return;
        
        const knob = joystick.querySelector('div');
        if (!knob) return;
        
        const rect = joystick.getBoundingClientRect();
        this.joystickCenter = {
            x: rect.left + rect.width / 2,
            y: rect.top + rect.height / 2
        };
        
        // Touch events for joystick
        joystick.addEventListener('touchstart', (event) => {
            event.preventDefault();
            this.joystickActive = true;
            
            const touch = event.touches[0];
            this.updateJoystick(touch.clientX, touch.clientY, knob);
        });
        
        joystick.addEventListener('touchmove', (event) => {
            event.preventDefault();
            
            if (this.joystickActive) {
                const touch = event.touches[0];
                this.updateJoystick(touch.clientX, touch.clientY, knob);
            }
        });
        
        joystick.addEventListener('touchend', (event) => {
            event.preventDefault();
            this.joystickActive = false;
            this.resetJoystick(knob);
        });
    }
    
    /**
     * Update virtual joystick
     */
    updateJoystick(x, y, knob) {
        const deltaX = x - this.joystickCenter.x;
        const deltaY = y - this.joystickCenter.y;
        const distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);
        const maxDistance = 35; // Half of joystick radius
        
        if (distance <= maxDistance) {
            this.joystickPosition.x = deltaX / maxDistance;
            this.joystickPosition.y = deltaY / maxDistance;
            knob.style.transform = `translate(${deltaX}px, ${deltaY}px)`;
        } else {
            const angle = Math.atan2(deltaY, deltaX);
            this.joystickPosition.x = Math.cos(angle);
            this.joystickPosition.y = Math.sin(angle);
            knob.style.transform = `translate(${Math.cos(angle) * maxDistance}px, ${Math.sin(angle) * maxDistance}px)`;
        }
        
        // Update movement based on joystick
        this.updateMovementFromJoystick();
    }
    
    /**
     * Reset virtual joystick
     */
    resetJoystick(knob) {
        this.joystickPosition.x = 0;
        this.joystickPosition.y = 0;
        knob.style.transform = 'translate(0px, 0px)';
        this.resetMovementControls();
    }
    
    /**
     * Update movement from keyboard keys
     */
    updateMovementFromKeys() {
        this.movement.forward = this.keys['KeyW'] || this.keys['ArrowUp'] || false;
        this.movement.backward = this.keys['KeyS'] || this.keys['ArrowDown'] || false;
        this.movement.left = this.keys['KeyA'] || this.keys['ArrowLeft'] || false;
        this.movement.right = this.keys['KeyD'] || this.keys['ArrowRight'] || false;
        this.movement.up = this.keys['Space'] || false;
        this.movement.down = this.keys['ShiftLeft'] || this.keys['ShiftRight'] || false;
    }
    
    /**
     * Update movement from virtual joystick
     */
    updateMovementFromJoystick() {
        const threshold = 0.2;
        
        this.movement.forward = this.joystickPosition.y < -threshold;
        this.movement.backward = this.joystickPosition.y > threshold;
        this.movement.left = this.joystickPosition.x < -threshold;
        this.movement.right = this.joystickPosition.x > threshold;
    }
    
    /**
     * Reset movement controls
     */
    resetMovementControls() {
        this.movement.forward = false;
        this.movement.backward = false;
        this.movement.left = false;
        this.movement.right = false;
        this.movement.up = false;
        this.movement.down = false;
    }
    
    /**
     * Update camera from mouse movement
     */
    updateCameraFromMouse(deltaX, deltaY) {
        if (!this.engine.camera) return;
        
        this.cameraRotation.y -= deltaX * this.sensitivity;
        this.cameraRotation.x -= deltaY * this.sensitivity;
        
        // Clamp vertical rotation
        this.cameraRotation.x = Math.max(-Math.PI / 2, Math.min(Math.PI / 2, this.cameraRotation.x));
        
        // Apply rotation to camera
        this.engine.camera.rotation.order = 'YXZ';
        this.engine.camera.rotation.y = this.cameraRotation.y;
        this.engine.camera.rotation.x = this.cameraRotation.x;
    }
    
    /**
     * Handle zoom
     */
    handleZoom(delta) {
        if (!this.engine.camera) return;
        
        const zoomSpeed = 0.1;
        const direction = this.engine.camera.getWorldDirection(new THREE.Vector3());
        
        if (delta > 0) {
            // Zoom out
            this.engine.camera.position.addScaledVector(direction, -zoomSpeed);
        } else {
            // Zoom in
            this.engine.camera.position.addScaledVector(direction, zoomSpeed);
        }
    }
    
    /**
     * Request pointer lock
     */
    requestPointerLock() {
        const canvas = this.engine.renderer.domElement;
        canvas.requestPointerLock = canvas.requestPointerLock || canvas.mozRequestPointerLock;
        
        if (canvas.requestPointerLock) {
            canvas.requestPointerLock();
        }
    }
    
    /**
     * Check if key is a game key
     */
    isGameKey(code) {
        const gameKeys = [
            'KeyW', 'KeyA', 'KeyS', 'KeyD',
            'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight',
            'Space', 'ShiftLeft', 'ShiftRight'
        ];
        
        return gameKeys.includes(code);
    }
    
    /**
     * Get movement vector
     */
    getMovementVector() {
        const vector = new THREE.Vector3();
        
        if (this.movement.forward) vector.z -= 1;
        if (this.movement.backward) vector.z += 1;
        if (this.movement.left) vector.x -= 1;
        if (this.movement.right) vector.x += 1;
        if (this.movement.up) vector.y += 1;
        if (this.movement.down) vector.y -= 1;
        
        return vector.normalize();
    }
    
    /**
     * Check if key is pressed
     */
    isKeyPressed(code) {
        return this.keys[code] || false;
    }
    
    /**
     * Get mouse position
     */
    getMousePosition() {
        return { x: this.mouse.x, y: this.mouse.y };
    }
    
    /**
     * Check if mouse is down
     */
    isMouseDown() {
        return this.mouse.isDown;
    }
}

