/**
 * CannaVille Pro - UI System
 * Manages user interface elements and interactions
 */

export class UISystem {
    constructor(engine) {
        this.engine = engine;
        this.container = null;
        this.panels = new Map();
        this.isInitialized = false;
        
        // Game data for UI display
        this.gameData = {
            money: 5000,
            energy: 100,
            level: 1,
            day: 1,
            experience: 0
        };
        
        // UI state
        this.activePanel = null;
        this.inspectMode = false;
        this.harvestMode = false;
    }
    
    /**
     * Initialize the UI system
     */
    async init() {
        console.log('🎨 Initializing UI System...');
        
        try {
            // Get UI container
            this.container = document.getElementById('ui-overlay');
            if (!this.container) {
                throw new Error('UI overlay container not found');
            }
            
            // Create UI panels
            this.createMainUI();
            this.createControlPanels();
            this.createMobileControls();
            this.createNotificationSystem();
            
            // Setup event listeners
            this.setupEventListeners();
            
            // Start update loop
            this.startUpdateLoop();
            
            this.isInitialized = true;
            console.log('✅ UI System initialized');
            
        } catch (error) {
            console.error('❌ Failed to initialize UI System:', error);
            throw error;
        }
    }
    
    /**
     * Create main UI elements
     */
    createMainUI() {
        // Main HUD container
        const mainHUD = document.createElement('div');
        mainHUD.id = 'main-hud';
        mainHUD.className = 'ui-element';
        mainHUD.style.cssText = `
            position: absolute;
            top: 20px;
            left: 20px;
            right: 20px;
            z-index: 100;
            pointer-events: none;
        `;
        
        // Top status bar
        const statusBar = this.createStatusBar();
        mainHUD.appendChild(statusBar);
        
        this.container.appendChild(mainHUD);
        this.panels.set('main', mainHUD);
    }
    
    /**
     * Create status bar
     */
    createStatusBar() {
        const statusBar = document.createElement('div');
        statusBar.id = 'status-bar';
        statusBar.style.cssText = `
            display: flex;
            justify-content: space-between;
            align-items: center;
            background: rgba(0, 0, 0, 0.7);
            backdrop-filter: blur(10px);
            border-radius: 15px;
            padding: 15px 20px;
            color: white;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            pointer-events: auto;
            margin-bottom: 20px;
        `;
        
        statusBar.innerHTML = `
            <div style="display: flex; gap: 20px; align-items: center;">
                <div style="display: flex; align-items: center; gap: 8px; background: rgba(76, 175, 80, 0.2); padding: 8px 12px; border-radius: 20px;">
                    <span>💰</span>
                    <span id="money-value" style="font-weight: bold; color: #4caf50;">$${this.gameData.money}</span>
                </div>
                <div style="display: flex; align-items: center; gap: 8px; background: rgba(255, 193, 7, 0.2); padding: 8px 12px; border-radius: 20px;">
                    <span>⚡</span>
                    <span id="energy-value" style="font-weight: bold; color: #ffc107;">${this.gameData.energy}</span>
                </div>
            </div>
            <div style="display: flex; gap: 20px; align-items: center;">
                <div style="display: flex; align-items: center; gap: 8px; background: rgba(156, 39, 176, 0.2); padding: 8px 12px; border-radius: 20px;">
                    <span>🏆</span>
                    <span style="font-weight: bold; color: #9c27b0;">Level <span id="level-value">${this.gameData.level}</span></span>
                </div>
                <div style="display: flex; align-items: center; gap: 8px; background: rgba(33, 150, 243, 0.2); padding: 8px 12px; border-radius: 20px;">
                    <span>📅</span>
                    <span style="font-weight: bold; color: #2196f3;">Day <span id="day-value">${this.gameData.day}</span></span>
                </div>
            </div>
            <div>
                <button id="settings-btn" style="background: rgba(255, 255, 255, 0.1); border: 1px solid rgba(255, 255, 255, 0.2); border-radius: 50%; width: 40px; height: 40px; color: white; cursor: pointer;">⚙️</button>
            </div>
        `;
        
        return statusBar;
    }
    
    /**
     * Create control panels
     */
    createControlPanels() {
        // Bottom control bar
        const controlBar = document.createElement('div');
        controlBar.id = 'control-bar';
        controlBar.className = 'ui-element';
        controlBar.style.cssText = `
            position: absolute;
            bottom: 20px;
            left: 50%;
            transform: translateX(-50%);
            display: flex;
            gap: 15px;
            background: rgba(0, 0, 0, 0.7);
            backdrop-filter: blur(10px);
            border-radius: 25px;
            padding: 15px 20px;
            pointer-events: auto;
            border: 1px solid rgba(255, 255, 255, 0.1);
        `;
        
        const buttons = [
            { id: 'home-btn', icon: '🏠', label: 'Home' },
            { id: 'inspect-btn', icon: '🔍', label: 'Inspect' },
            { id: 'harvest-btn', icon: '✂️', label: 'Harvest' },
            { id: 'shop-btn', icon: '🛒', label: 'Shop' }
        ];
        
        buttons.forEach(btn => {
            const button = document.createElement('button');
            button.id = btn.id;
            button.style.cssText = `
                background: rgba(255, 255, 255, 0.1);
                border: 1px solid rgba(255, 255, 255, 0.2);
                border-radius: 20px;
                padding: 12px 16px;
                color: white;
                cursor: pointer;
                display: flex;
                flex-direction: column;
                align-items: center;
                gap: 5px;
                min-width: 70px;
                transition: all 0.3s ease;
                font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            `;
            
            button.innerHTML = `
                <span style="font-size: 1.5em;">${btn.icon}</span>
                <span style="font-size: 0.8em; font-weight: 500;">${btn.label}</span>
            `;
            
            controlBar.appendChild(button);
        });
        
        this.container.appendChild(controlBar);
        this.panels.set('controls', controlBar);
    }
    
    /**
     * Create mobile controls
     */
    createMobileControls() {
        if (!this.engine.deviceInfo.isMobile) {
            return;
        }
        
        // Virtual joystick for mobile
        const joystick = document.createElement('div');
        joystick.id = 'virtual-joystick';
        joystick.style.cssText = `
            position: absolute;
            bottom: 100px;
            left: 30px;
            width: 120px;
            height: 120px;
            background: rgba(255, 255, 255, 0.1);
            border: 2px solid rgba(255, 255, 255, 0.3);
            border-radius: 50%;
            pointer-events: auto;
            display: flex;
            align-items: center;
            justify-content: center;
        `;
        
        const knob = document.createElement('div');
        knob.style.cssText = `
            width: 50px;
            height: 50px;
            background: rgba(76, 175, 80, 0.8);
            border-radius: 50%;
            transition: all 0.1s ease;
        `;
        
        joystick.appendChild(knob);
        this.container.appendChild(joystick);
        this.panels.set('joystick', joystick);
    }
    
    /**
     * Create notification system
     */
    createNotificationSystem() {
        const notificationContainer = document.createElement('div');
        notificationContainer.id = 'notification-container';
        notificationContainer.style.cssText = `
            position: absolute;
            top: 100px;
            right: 20px;
            width: 300px;
            z-index: 200;
            pointer-events: none;
        `;
        
        this.container.appendChild(notificationContainer);
        this.panels.set('notifications', notificationContainer);
    }
    
    /**
     * Setup event listeners
     */
    setupEventListeners() {
        // Settings button
        const settingsBtn = document.getElementById('settings-btn');
        if (settingsBtn) {
            settingsBtn.addEventListener('click', () => this.toggleSettingsPanel());
        }
        
        // Control buttons
        const homeBtn = document.getElementById('home-btn');
        if (homeBtn) {
            homeBtn.addEventListener('click', () => this.goHome());
        }
        
        const inspectBtn = document.getElementById('inspect-btn');
        if (inspectBtn) {
            inspectBtn.addEventListener('click', () => this.toggleInspectMode());
        }
        
        const harvestBtn = document.getElementById('harvest-btn');
        if (harvestBtn) {
            harvestBtn.addEventListener('click', () => this.toggleHarvestMode());
        }
        
        const shopBtn = document.getElementById('shop-btn');
        if (shopBtn) {
            shopBtn.addEventListener('click', () => this.openShop());
        }
    }
    
    /**
     * Start update loop
     */
    startUpdateLoop() {
        const update = () => {
            if (this.isInitialized) {
                this.updateUI();
            }
            requestAnimationFrame(update);
        };
        update();
    }
    
    /**
     * Update UI elements
     */
    updateUI() {
        // Update resource displays
        const moneyValue = document.getElementById('money-value');
        if (moneyValue) {
            moneyValue.textContent = `$${this.gameData.money.toLocaleString()}`;
        }
        
        const energyValue = document.getElementById('energy-value');
        if (energyValue) {
            energyValue.textContent = this.gameData.energy;
        }
        
        const levelValue = document.getElementById('level-value');
        if (levelValue) {
            levelValue.textContent = this.gameData.level;
        }
        
        const dayValue = document.getElementById('day-value');
        if (dayValue) {
            dayValue.textContent = this.gameData.day;
        }
    }
    
    /**
     * Show notification
     */
    showNotification(message, type = 'info', duration = 3000) {
        const container = this.panels.get('notifications');
        if (!container) return;
        
        const notification = document.createElement('div');
        notification.style.cssText = `
            background: rgba(0, 0, 0, 0.8);
            backdrop-filter: blur(10px);
            border-radius: 10px;
            padding: 15px;
            margin-bottom: 10px;
            color: white;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            pointer-events: auto;
            transform: translateX(100%);
            transition: transform 0.3s ease;
            border-left: 4px solid ${type === 'success' ? '#4caf50' : type === 'error' ? '#f44336' : '#2196f3'};
        `;
        
        notification.textContent = message;
        container.appendChild(notification);
        
        // Animate in
        setTimeout(() => {
            notification.style.transform = 'translateX(0)';
        }, 100);
        
        // Auto remove
        setTimeout(() => {
            notification.style.transform = 'translateX(100%)';
            setTimeout(() => {
                if (notification.parentNode) {
                    notification.parentNode.removeChild(notification);
                }
            }, 300);
        }, duration);
    }
    
    /**
     * Toggle settings panel
     */
    toggleSettingsPanel() {
        console.log('Settings panel toggled');
        this.showNotification('Settings panel coming soon!', 'info');
    }
    
    /**
     * Go to home position
     */
    goHome() {
        console.log('Going home');
        if (this.engine && this.engine.camera) {
            this.engine.camera.position.set(0, 3, 8);
            this.engine.camera.lookAt(0, 1, 0);
        }
        this.showNotification('Returned to home position', 'success');
    }
    
    /**
     * Toggle inspect mode
     */
    toggleInspectMode() {
        this.inspectMode = !this.inspectMode;
        console.log('Inspect mode:', this.inspectMode);
        this.showNotification(
            this.inspectMode ? 'Inspect mode enabled - click plants to analyze' : 'Inspect mode disabled',
            'info'
        );
    }
    
    /**
     * Toggle harvest mode
     */
    toggleHarvestMode() {
        this.harvestMode = !this.harvestMode;
        console.log('Harvest mode:', this.harvestMode);
        this.showNotification(
            this.harvestMode ? 'Harvest mode enabled - click mature plants to harvest' : 'Harvest mode disabled',
            'info'
        );
    }
    
    /**
     * Open shop
     */
    openShop() {
        console.log('Opening shop');
        this.showNotification('Shop coming soon!', 'info');
    }
    
    /**
     * Show transition effect
     */
    showTransition(message) {
        const transition = document.createElement('div');
        transition.id = 'transition-overlay';
        transition.style.cssText = `
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0, 0, 0, 0.8);
            display: flex;
            align-items: center;
            justify-content: center;
            z-index: 1000;
            color: white;
            font-size: 1.5rem;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
        `;
        
        transition.textContent = message;
        document.body.appendChild(transition);
    }
    
    /**
     * Hide transition effect
     */
    hideTransition() {
        const transition = document.getElementById('transition-overlay');
        if (transition) {
            transition.remove();
        }
    }
    
    /**
     * Show error message
     */
    showError(message) {
        this.showNotification(message, 'error', 5000);
    }
    
    /**
     * Update game data
     */
    updateGameData(data) {
        Object.assign(this.gameData, data);
    }
}

