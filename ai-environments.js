/**
 * CannaVille Pro - AI Environments
 * Intelligent environment management and optimization system
 */

class AIEnvironments {
    constructor(game) {
        this.game = game;
        this.currentEnvironment = 'indoor';
        this.environmentData = {};
        this.aiOptimizer = new EnvironmentOptimizer();
        this.weatherSystem = new WeatherSystem();
        this.lightingSystem = new LightingSystem(game.scene);
        
        // Environment configurations
        this.environments = {
            indoor: {
                name: 'Indoor Grow Room',
                description: 'Controlled indoor environment with artificial lighting',
                lighting: {
                    type: 'artificial',
                    intensity: 0.8,
                    spectrum: 'full',
                    schedule: '18/6'
                },
                climate: {
                    temperature: { min: 70, max: 78, optimal: 74 },
                    humidity: { min: 40, max: 60, optimal: 50 },
                    co2: { min: 400, max: 1500, optimal: 1200 },
                    airflow: { min: 20, max: 80, optimal: 50 }
                },
                background: 0x2c2c2c,
                ambientLight: 0.3,
                features: ['climate_control', 'artificial_lighting', 'co2_injection']
            },
            outdoor: {
                name: 'Outdoor Garden',
                description: 'Natural outdoor growing environment',
                lighting: {
                    type: 'natural',
                    intensity: 1.0,
                    spectrum: 'solar',
                    schedule: 'natural'
                },
                climate: {
                    temperature: { min: 60, max: 85, optimal: 75 },
                    humidity: { min: 30, max: 70, optimal: 45 },
                    co2: { min: 350, max: 420, optimal: 400 },
                    airflow: { min: 5, max: 40, optimal: 15 }
                },
                background: 0x87CEEB,
                ambientLight: 0.6,
                features: ['natural_sunlight', 'weather_dependent', 'seasonal_changes']
            },
            greenhouse: {
                name: 'Climate-Controlled Greenhouse',
                description: 'Hybrid environment combining natural and artificial elements',
                lighting: {
                    type: 'hybrid',
                    intensity: 0.9,
                    spectrum: 'enhanced',
                    schedule: 'adaptive'
                },
                climate: {
                    temperature: { min: 68, max: 82, optimal: 76 },
                    humidity: { min: 45, max: 65, optimal: 55 },
                    co2: { min: 400, max: 1200, optimal: 800 },
                    airflow: { min: 15, max: 60, optimal: 35 }
                },
                background: 0x90EE90,
                ambientLight: 0.5,
                features: ['natural_light', 'climate_control', 'weather_protection']
            }
        };
        
        // Current environment parameters
        this.currentParameters = {
            temperature: 72,
            humidity: 50,
            co2Level: 400,
            lightIntensity: 75,
            lightSchedule: '18/6',
            airflow: 30
        };
        
        // AI learning data
        this.performanceHistory = [];
        this.optimizationSuggestions = [];
        
        this.init();
    }
    
    async init() {
        console.log('🤖 Initializing AI Environments...');
        
        try {
            // Initialize environment systems
            await this.initializeEnvironmentSystems();
            
            // Setup AI optimization
            this.setupAIOptimization();
            
            // Setup environment monitoring
            this.setupEnvironmentMonitoring();
            
            // Load historical data
            this.loadHistoricalData();
            
            console.log('✅ AI Environments initialized successfully');
            
        } catch (error) {
            console.error('❌ Failed to initialize AI Environments:', error);
        }
    }
    
    async initializeEnvironmentSystems() {
        // Initialize weather system
        await this.weatherSystem.init();
        
        // Initialize lighting system
        await this.lightingSystem.init();
        
        // Set initial environment
        this.switchEnvironment(this.currentEnvironment);
    }
    
    setupAIOptimization() {
        // Run AI optimization every 30 seconds
        setInterval(() => {
            this.runAIOptimization();
        }, 30000);
        
        // Monitor plant health and adjust accordingly
        setInterval(() => {
            this.monitorAndAdjust();
        }, 10000);
    }
    
    setupEnvironmentMonitoring() {
        // Monitor environmental parameters
        setInterval(() => {
            this.recordEnvironmentData();
        }, 5000);
        
        // Check for alerts
        setInterval(() => {
            this.checkEnvironmentAlerts();
        }, 15000);
    }
    
    switchEnvironment(environmentType) {
        console.log(`🏠 Switching to ${environmentType} environment`);
        
        const envConfig = this.environments[environmentType];
        if (!envConfig) {
            console.error(`Unknown environment type: ${environmentType}`);
            return;
        }
        
        this.currentEnvironment = environmentType;
        
        // Update scene background
        this.game.scene.background = new THREE.Color(envConfig.background);
        
        // Update lighting
        this.lightingSystem.setEnvironmentLighting(envConfig);
        
        // Update climate parameters
        this.updateClimateParameters(envConfig);
        
        // Update UI
        this.updateEnvironmentUI(envConfig);
        
        // Generate AI recommendations for new environment
        this.generateEnvironmentRecommendations(envConfig);
        
        this.game.notificationManager.show(
            'Environment Changed 🏠',
            `Switched to ${envConfig.name}`,
            'info'
        );
    }
    
    updateClimateParameters(envConfig) {
        const climate = envConfig.climate;
        
        // Set optimal parameters for the environment
        this.currentParameters.temperature = climate.temperature.optimal;
        this.currentParameters.humidity = climate.humidity.optimal;
        this.currentParameters.co2Level = climate.co2.optimal;
        this.currentParameters.airflow = climate.airflow.optimal;
        
        // Update lighting based on environment
        const lighting = envConfig.lighting;
        this.currentParameters.lightIntensity = lighting.intensity * 100;
        this.currentParameters.lightSchedule = lighting.schedule;
    }
    
    updateEnvironmentUI(envConfig) {
        // Update environment controls to reflect current settings
        const controls = {
            'temperature': this.currentParameters.temperature,
            'humidity': this.currentParameters.humidity,
            'co2-level': this.currentParameters.co2Level,
            'light-intensity': this.currentParameters.lightIntensity
        };
        
        Object.entries(controls).forEach(([controlId, value]) => {
            const control = document.getElementById(controlId);
            const valueDisplay = document.getElementById(controlId.replace('-', '-') + '-value');
            
            if (control) {
                control.value = value;
                
                if (valueDisplay) {
                    let displayValue = value;
                    switch (controlId) {
                        case 'temperature':
                            displayValue = `${value}°F`;
                            break;
                        case 'humidity':
                        case 'light-intensity':
                            displayValue = `${value}%`;
                            break;
                        case 'co2-level':
                            displayValue = `${value} ppm`;
                            break;
                    }
                    valueDisplay.textContent = displayValue;
                }
            }
        });
        
        // Update light schedule
        const lightScheduleSelect = document.getElementById('light-schedule');
        if (lightScheduleSelect) {
            lightScheduleSelect.value = this.currentParameters.lightSchedule;
        }
    }
    
    runAIOptimization() {
        console.log('🧠 Running AI optimization...');
        
        // Analyze current plant health and growth
        const plantHealthData = this.analyzePlantHealth();
        
        // Generate optimization suggestions
        const suggestions = this.aiOptimizer.generateOptimizations(
            this.currentParameters,
            plantHealthData,
            this.performanceHistory
        );
        
        // Apply automatic optimizations
        this.applyAutomaticOptimizations(suggestions);
        
        // Store suggestions for user review
        this.optimizationSuggestions = suggestions;
        
        // Update UI with suggestions
        this.updateOptimizationUI(suggestions);
    }
    
    analyzePlantHealth() {
        const plants = this.game.gameState.plants;
        const healthData = {
            averageHealth: 0,
            growthRate: 0,
            stageDistribution: {},
            problemAreas: []
        };
        
        if (plants.length === 0) {
            return healthData;
        }
        
        // Calculate average health
        let totalHealth = 0;
        const stageCount = {};
        
        plants.forEach(plant => {
            totalHealth += plant.health;
            stageCount[plant.stage] = (stageCount[plant.stage] || 0) + 1;
            
            // Check for problems
            if (plant.health < 0.7) {
                healthData.problemAreas.push({
                    plantId: plant.id,
                    issue: 'low_health',
                    severity: 1 - plant.health
                });
            }
        });
        
        healthData.averageHealth = totalHealth / plants.length;
        healthData.stageDistribution = stageCount;
        
        // Calculate growth rate based on stage progression
        healthData.growthRate = this.calculateGrowthRate();
        
        return healthData;
    }
    
    calculateGrowthRate() {
        // Simplified growth rate calculation
        // In a real implementation, this would track stage changes over time
        const recentHistory = this.performanceHistory.slice(-10);
        if (recentHistory.length < 2) return 0.5;
        
        const growthEvents = recentHistory.filter(entry => 
            entry.event === 'stage_change'
        ).length;
        
        return Math.min(1.0, growthEvents / recentHistory.length);
    }
    
    applyAutomaticOptimizations(suggestions) {
        suggestions.forEach(suggestion => {
            if (suggestion.autoApply && suggestion.confidence > 0.8) {
                this.applyOptimization(suggestion);
            }
        });
    }
    
    applyOptimization(optimization) {
        console.log(`🔧 Applying optimization: ${optimization.type}`);
        
        switch (optimization.type) {
            case 'temperature_adjustment':
                this.adjustTemperature(optimization.value);
                break;
            case 'humidity_adjustment':
                this.adjustHumidity(optimization.value);
                break;
            case 'co2_adjustment':
                this.adjustCO2(optimization.value);
                break;
            case 'light_adjustment':
                this.adjustLighting(optimization.value);
                break;
            case 'schedule_change':
                this.changeLightSchedule(optimization.value);
                break;
        }
        
        // Record the optimization
        this.recordOptimization(optimization);
    }
    
    adjustTemperature(newValue) {
        this.currentParameters.temperature = newValue;
        this.updateEnvironmentParameter('temperature', newValue);
        
        this.game.notificationManager.show(
            'Temperature Adjusted 🌡️',
            `AI optimized temperature to ${newValue}°F`,
            'info'
        );
    }
    
    adjustHumidity(newValue) {
        this.currentParameters.humidity = newValue;
        this.updateEnvironmentParameter('humidity', newValue);
        
        this.game.notificationManager.show(
            'Humidity Adjusted 💨',
            `AI optimized humidity to ${newValue}%`,
            'info'
        );
    }
    
    adjustCO2(newValue) {
        this.currentParameters.co2Level = newValue;
        this.updateEnvironmentParameter('co2-level', newValue);
        
        this.game.notificationManager.show(
            'CO₂ Adjusted 🌬️',
            `AI optimized CO₂ to ${newValue} ppm`,
            'info'
        );
    }
    
    adjustLighting(newValue) {
        this.currentParameters.lightIntensity = newValue;
        this.updateEnvironmentParameter('light-intensity', newValue);
        this.lightingSystem.setIntensity(newValue / 100);
        
        this.game.notificationManager.show(
            'Lighting Adjusted 💡',
            `AI optimized light intensity to ${newValue}%`,
            'info'
        );
    }
    
    changeLightSchedule(newSchedule) {
        this.currentParameters.lightSchedule = newSchedule;
        
        const lightScheduleSelect = document.getElementById('light-schedule');
        if (lightScheduleSelect) {
            lightScheduleSelect.value = newSchedule;
        }
        
        this.game.notificationManager.show(
            'Light Schedule Changed ⏰',
            `AI optimized light schedule to ${newSchedule}`,
            'info'
        );
    }
    
    updateEnvironmentParameter(parameterId, value) {
        const control = document.getElementById(parameterId);
        const valueDisplay = document.getElementById(parameterId.replace('-', '-') + '-value');
        
        if (control) {
            control.value = value;
            
            if (valueDisplay) {
                let displayValue = value;
                switch (parameterId) {
                    case 'temperature':
                        displayValue = `${value}°F`;
                        break;
                    case 'humidity':
                    case 'light-intensity':
                        displayValue = `${value}%`;
                        break;
                    case 'co2-level':
                        displayValue = `${value} ppm`;
                        break;
                }
                valueDisplay.textContent = displayValue;
            }
        }
        
        // Trigger change event for game logic
        if (this.game.updateEnvironmentParameter) {
            this.game.updateEnvironmentParameter(parameterId, value);
        }
    }
    
    monitorAndAdjust() {
        // Monitor plant health and make micro-adjustments
        const healthData = this.analyzePlantHealth();
        
        if (healthData.averageHealth < 0.6) {
            // Plants are struggling, make emergency adjustments
            this.makeEmergencyAdjustments(healthData);
        } else if (healthData.averageHealth > 0.9 && healthData.growthRate > 0.8) {
            // Plants are thriving, optimize for efficiency
            this.optimizeForEfficiency();
        }
    }
    
    makeEmergencyAdjustments(healthData) {
        console.log('🚨 Making emergency environment adjustments');
        
        const envConfig = this.environments[this.currentEnvironment];
        
        // Adjust to optimal parameters
        this.adjustTemperature(envConfig.climate.temperature.optimal);
        this.adjustHumidity(envConfig.climate.humidity.optimal);
        this.adjustCO2(envConfig.climate.co2.optimal);
        
        this.game.notificationManager.show(
            'Emergency Adjustments Made! 🚨',
            'AI detected plant stress and optimized environment',
            'warning'
        );
    }
    
    optimizeForEfficiency() {
        console.log('⚡ Optimizing for efficiency');
        
        // Slightly reduce resource usage while maintaining health
        const currentTemp = this.currentParameters.temperature;
        const currentCO2 = this.currentParameters.co2Level;
        
        this.adjustTemperature(Math.max(70, currentTemp - 1));
        this.adjustCO2(Math.max(400, currentCO2 - 50));
        
        this.game.notificationManager.show(
            'Efficiency Optimized ⚡',
            'AI reduced resource usage while maintaining plant health',
            'success'
        );
    }
    
    recordEnvironmentData() {
        const data = {
            timestamp: Date.now(),
            environment: this.currentEnvironment,
            parameters: { ...this.currentParameters },
            plantHealth: this.analyzePlantHealth(),
            weather: this.weatherSystem.getCurrentWeather()
        };
        
        this.environmentData[data.timestamp] = data;
        this.performanceHistory.push(data);
        
        // Keep only last 100 entries
        if (this.performanceHistory.length > 100) {
            this.performanceHistory.shift();
        }
    }
    
    checkEnvironmentAlerts() {
        const envConfig = this.environments[this.currentEnvironment];
        const alerts = [];
        
        // Check temperature
        if (this.currentParameters.temperature < envConfig.climate.temperature.min ||
            this.currentParameters.temperature > envConfig.climate.temperature.max) {
            alerts.push({
                type: 'temperature',
                severity: 'warning',
                message: 'Temperature is outside optimal range'
            });
        }
        
        // Check humidity
        if (this.currentParameters.humidity < envConfig.climate.humidity.min ||
            this.currentParameters.humidity > envConfig.climate.humidity.max) {
            alerts.push({
                type: 'humidity',
                severity: 'warning',
                message: 'Humidity is outside optimal range'
            });
        }
        
        // Check CO2
        if (this.currentParameters.co2Level < envConfig.climate.co2.min) {
            alerts.push({
                type: 'co2',
                severity: 'info',
                message: 'CO₂ levels could be increased for better growth'
            });
        }
        
        // Show alerts
        alerts.forEach(alert => {
            this.game.notificationManager.show(
                `Environment Alert: ${alert.type}`,
                alert.message,
                alert.severity
            );
        });
    }
    
    generateEnvironmentRecommendations(envConfig) {
        const recommendations = [];
        
        // Generate recommendations based on environment type
        switch (this.currentEnvironment) {
            case 'indoor':
                recommendations.push('Consider CO₂ supplementation for enhanced growth');
                recommendations.push('Monitor humidity closely in sealed environment');
                recommendations.push('Ensure adequate ventilation for air circulation');
                break;
            case 'outdoor':
                recommendations.push('Watch weather forecasts for temperature extremes');
                recommendations.push('Consider wind protection for young plants');
                recommendations.push('Monitor natural light cycles for flowering timing');
                break;
            case 'greenhouse':
                recommendations.push('Balance natural and artificial lighting');
                recommendations.push('Use thermal mass for temperature stability');
                recommendations.push('Implement automated climate control systems');
                break;
        }
        
        // Update recommendations UI
        this.updateRecommendationsUI(recommendations);
    }
    
    updateRecommendationsUI(recommendations) {
        const recommendationsList = document.getElementById('recommendations-list');
        if (recommendationsList) {
            recommendationsList.innerHTML = '';
            recommendations.forEach(rec => {
                const li = document.createElement('li');
                li.textContent = rec;
                recommendationsList.appendChild(li);
            });
        }
    }
    
    updateOptimizationUI(suggestions) {
        // Update AI optimization button with suggestions count
        const optimizeBtn = document.getElementById('ai-optimize');
        if (optimizeBtn && suggestions.length > 0) {
            optimizeBtn.textContent = `🤖 AI Optimize (${suggestions.length} suggestions)`;
            optimizeBtn.classList.add('has-suggestions');
        }
    }
    
    recordOptimization(optimization) {
        this.performanceHistory.push({
            timestamp: Date.now(),
            event: 'optimization_applied',
            type: optimization.type,
            value: optimization.value,
            confidence: optimization.confidence
        });
    }
    
    loadHistoricalData() {
        // In a real implementation, this would load from a database
        // For now, generate some sample historical data
        this.generateSampleHistory();
    }
    
    generateSampleHistory() {
        const now = Date.now();
        const oneHour = 60 * 60 * 1000;
        
        for (let i = 24; i >= 0; i--) {
            const timestamp = now - (i * oneHour);
            this.performanceHistory.push({
                timestamp,
                environment: this.currentEnvironment,
                parameters: {
                    temperature: 72 + (Math.random() - 0.5) * 4,
                    humidity: 50 + (Math.random() - 0.5) * 10,
                    co2Level: 400 + Math.random() * 200,
                    lightIntensity: 75 + (Math.random() - 0.5) * 10
                },
                plantHealth: {
                    averageHealth: 0.7 + Math.random() * 0.3,
                    growthRate: 0.4 + Math.random() * 0.4
                }
            });
        }
    }
    
    getEnvironmentInfo() {
        return {
            current: this.currentEnvironment,
            config: this.environments[this.currentEnvironment],
            parameters: this.currentParameters,
            suggestions: this.optimizationSuggestions
        };
    }
    
    exportEnvironmentData() {
        return {
            history: this.performanceHistory,
            currentParameters: this.currentParameters,
            optimizations: this.optimizationSuggestions
        };
    }
}

// Supporting classes
class EnvironmentOptimizer {
    generateOptimizations(currentParams, healthData, history) {
        const suggestions = [];
        
        // Analyze trends and generate suggestions
        if (healthData.averageHealth < 0.8) {
            suggestions.push({
                type: 'humidity_adjustment',
                value: Math.min(60, currentParams.humidity + 5),
                confidence: 0.7,
                autoApply: false,
                reason: 'Low plant health detected, increasing humidity may help'
            });
        }
        
        if (healthData.growthRate < 0.5) {
            suggestions.push({
                type: 'co2_adjustment',
                value: Math.min(1200, currentParams.co2Level + 100),
                confidence: 0.8,
                autoApply: true,
                reason: 'Slow growth rate, CO₂ supplementation recommended'
            });
        }
        
        return suggestions;
    }
}

class WeatherSystem {
    constructor() {
        this.currentWeather = {
            temperature: 75,
            humidity: 45,
            windSpeed: 5,
            cloudCover: 0.3,
            precipitation: 0
        };
    }
    
    async init() {
        // Initialize weather system
        this.updateWeather();
        
        // Update weather every 5 minutes
        setInterval(() => {
            this.updateWeather();
        }, 5 * 60 * 1000);
    }
    
    updateWeather() {
        // Simulate weather changes
        this.currentWeather.temperature += (Math.random() - 0.5) * 2;
        this.currentWeather.humidity += (Math.random() - 0.5) * 5;
        this.currentWeather.windSpeed = Math.max(0, this.currentWeather.windSpeed + (Math.random() - 0.5) * 2);
        this.currentWeather.cloudCover = Math.max(0, Math.min(1, this.currentWeather.cloudCover + (Math.random() - 0.5) * 0.1));
    }
    
    getCurrentWeather() {
        return { ...this.currentWeather };
    }
}

class LightingSystem {
    constructor(scene) {
        this.scene = scene;
        this.lights = {};
        this.currentIntensity = 0.8;
    }
    
    async init() {
        // Initialize lighting system
        this.setupLights();
    }
    
    setupLights() {
        // Get existing lights from scene
        this.scene.traverse((object) => {
            if (object.isLight) {
                this.lights[object.type] = object;
            }
        });
    }
    
    setEnvironmentLighting(envConfig) {
        // Update ambient light
        if (this.lights.AmbientLight) {
            this.lights.AmbientLight.intensity = envConfig.ambientLight;
        }
        
        // Update directional light based on environment
        if (this.lights.DirectionalLight) {
            switch (envConfig.lighting.type) {
                case 'natural':
                    this.lights.DirectionalLight.intensity = 0.8;
                    this.lights.DirectionalLight.color.setHex(0xffffff);
                    break;
                case 'artificial':
                    this.lights.DirectionalLight.intensity = 0.6;
                    this.lights.DirectionalLight.color.setHex(0xff00ff);
                    break;
                case 'hybrid':
                    this.lights.DirectionalLight.intensity = 0.7;
                    this.lights.DirectionalLight.color.setHex(0xffff99);
                    break;
            }
        }
    }
    
    setIntensity(intensity) {
        this.currentIntensity = intensity;
        
        // Update all lights
        Object.values(this.lights).forEach(light => {
            if (light.intensity !== undefined) {
                light.intensity = light.intensity * intensity;
            }
        });
    }
}

// Export for use in main game
window.AIEnvironments = AIEnvironments;

