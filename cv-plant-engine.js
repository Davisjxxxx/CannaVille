/**
 * CannaVille Pro - CV Plant Engine & Smart Room AI
 * Computer vision plant analysis and smart room automation
 */

// Computer Vision Plant Engine
class CVPlantEngine {
    constructor(game) {
        this.game = game;
        this.isAnalyzing = false;
        this.analysisHistory = [];
        this.healthMetrics = {
            leafColor: 0.8,
            leafSize: 0.7,
            stemThickness: 0.9,
            overallHealth: 0.8,
            growthRate: 0.6,
            pestPresence: 0.0,
            diseaseIndicators: 0.1
        };
        
        // Plant health indicators
        this.healthIndicators = {
            excellent: { min: 0.9, color: '#32CD32', message: 'Excellent health!' },
            good: { min: 0.7, color: '#90EE90', message: 'Good health' },
            fair: { min: 0.5, color: '#FFD700', message: 'Fair health - needs attention' },
            poor: { min: 0.3, color: '#FFA500', message: 'Poor health - immediate action needed' },
            critical: { min: 0.0, color: '#FF4757', message: 'Critical condition!' }
        };
        
        // Disease detection patterns
        this.diseasePatterns = {
            powdery_mildew: {
                symptoms: ['white_spots', 'leaf_discoloration'],
                severity: 'moderate',
                treatment: 'neem_oil'
            },
            spider_mites: {
                symptoms: ['tiny_spots', 'webbing', 'leaf_yellowing'],
                severity: 'high',
                treatment: 'ladybugs'
            },
            nutrient_deficiency: {
                symptoms: ['leaf_yellowing', 'stunted_growth'],
                severity: 'moderate',
                treatment: 'nutrients'
            },
            overwatering: {
                symptoms: ['leaf_drooping', 'root_rot'],
                severity: 'high',
                treatment: 'reduce_watering'
            }
        };
        
        this.init();
    }
    
    async init() {
        console.log('🔬 Initializing CV Plant Engine...');
        
        try {
            // Initialize computer vision models
            await this.loadCVModels();
            
            // Setup analysis intervals
            this.setupAnalysisSchedule();
            
            // Initialize health monitoring
            this.startHealthMonitoring();
            
            console.log('✅ CV Plant Engine initialized successfully');
            
        } catch (error) {
            console.error('❌ Failed to initialize CV Plant Engine:', error);
        }
    }
    
    async loadCVModels() {
        // In a real implementation, this would load actual ML models
        // For demo purposes, we'll simulate model loading
        console.log('📦 Loading computer vision models...');
        
        // Simulate loading time
        await new Promise(resolve => setTimeout(resolve, 1000));
        
        this.models = {
            healthDetection: { loaded: true, accuracy: 0.92 },
            diseaseDetection: { loaded: true, accuracy: 0.88 },
            growthStageClassification: { loaded: true, accuracy: 0.95 },
            pestDetection: { loaded: true, accuracy: 0.85 }
        };
        
        console.log('✅ CV models loaded successfully');
    }
    
    setupAnalysisSchedule() {
        // Analyze plants every 30 seconds
        setInterval(() => {
            this.analyzeAllPlants();
        }, 30000);
        
        // Deep analysis every 5 minutes
        setInterval(() => {
            this.performDeepAnalysis();
        }, 5 * 60 * 1000);
    }
    
    startHealthMonitoring() {
        // Monitor plant health continuously
        setInterval(() => {
            this.updateHealthMetrics();
        }, 10000);
    }
    
    async analyzeAllPlants() {
        if (this.isAnalyzing) return;
        
        this.isAnalyzing = true;
        
        try {
            const plants = this.game.gameState.plants;
            const analysisResults = [];
            
            for (const plant of plants) {
                const result = await this.analyzePlant(plant);
                analysisResults.push(result);
            }
            
            // Update game state with analysis results
            this.updateGameStateWithAnalysis(analysisResults);
            
            // Store analysis history
            this.analysisHistory.push({
                timestamp: Date.now(),
                results: analysisResults,
                overallHealth: this.calculateOverallHealth(analysisResults)
            });
            
            // Limit history size
            if (this.analysisHistory.length > 100) {
                this.analysisHistory.shift();
            }
            
        } catch (error) {
            console.error('Plant analysis error:', error);
        } finally {
            this.isAnalyzing = false;
        }
    }
    
    async analyzePlant(plant) {
        // Simulate computer vision analysis
        const analysis = {
            plantId: plant.id,
            timestamp: Date.now(),
            stage: plant.stage,
            health: this.calculatePlantHealth(plant),
            diseases: this.detectDiseases(plant),
            pests: this.detectPests(plant),
            growth: this.analyzeGrowth(plant),
            recommendations: []
        };
        
        // Generate recommendations based on analysis
        analysis.recommendations = this.generateRecommendations(analysis);
        
        return analysis;
    }
    
    calculatePlantHealth(plant) {
        // Simulate health calculation based on various factors
        let health = plant.health;
        
        // Environmental factors
        const envParams = this.game.aiEnvironments?.currentParameters || {};
        
        // Temperature impact
        if (envParams.temperature) {
            if (envParams.temperature < 65 || envParams.temperature > 85) {
                health *= 0.9;
            }
        }
        
        // Humidity impact
        if (envParams.humidity) {
            if (envParams.humidity < 40 || envParams.humidity > 70) {
                health *= 0.95;
            }
        }
        
        // Age impact
        const stageOptimalAge = {
            seedling: 14,
            vegetative: 28,
            flowering: 56,
            harvest: 70
        };
        
        const optimalAge = stageOptimalAge[plant.stage] || 30;
        if (plant.age > optimalAge * 1.5) {
            health *= 0.8; // Overaged plant
        }
        
        // Random variation for realism
        health += (Math.random() - 0.5) * 0.1;
        
        return Math.max(0, Math.min(1, health));
    }
    
    detectDiseases(plant) {
        const diseases = [];
        
        // Simulate disease detection based on plant health and environment
        if (plant.health < 0.7) {
            const diseaseChance = (1 - plant.health) * 0.3;
            
            if (Math.random() < diseaseChance) {
                const diseaseTypes = Object.keys(this.diseasePatterns);
                const randomDisease = diseaseTypes[Math.floor(Math.random() * diseaseTypes.length)];
                
                diseases.push({
                    type: randomDisease,
                    confidence: 0.6 + Math.random() * 0.3,
                    severity: this.diseasePatterns[randomDisease].severity,
                    treatment: this.diseasePatterns[randomDisease].treatment
                });
            }
        }
        
        return diseases;
    }
    
    detectPests(plant) {
        const pests = [];
        
        // Simulate pest detection
        const pestChance = 0.05; // 5% chance
        
        if (Math.random() < pestChance) {
            const pestTypes = ['spider_mites', 'aphids', 'thrips', 'whiteflies'];
            const randomPest = pestTypes[Math.floor(Math.random() * pestTypes.length)];
            
            pests.push({
                type: randomPest,
                confidence: 0.7 + Math.random() * 0.2,
                severity: Math.random() > 0.5 ? 'low' : 'moderate',
                location: 'leaves'
            });
        }
        
        return pests;
    }
    
    analyzeGrowth(plant) {
        const expectedSize = this.getExpectedSize(plant);
        const actualSize = plant.size || 0.5;
        
        return {
            expectedSize,
            actualSize,
            growthRate: actualSize / expectedSize,
            isStunted: actualSize < expectedSize * 0.8,
            isOvergrown: actualSize > expectedSize * 1.2
        };
    }
    
    getExpectedSize(plant) {
        const stageExpectedSizes = {
            seedling: 0.3,
            vegetative: 0.6,
            flowering: 0.9,
            harvest: 1.0
        };
        
        return stageExpectedSizes[plant.stage] || 0.5;
    }
    
    generateRecommendations(analysis) {
        const recommendations = [];
        
        // Health-based recommendations
        if (analysis.health < 0.6) {
            recommendations.push({
                type: 'health',
                priority: 'high',
                action: 'immediate_attention',
                message: 'Plant health is critical - check environment and nutrients'
            });
        } else if (analysis.health < 0.8) {
            recommendations.push({
                type: 'health',
                priority: 'medium',
                action: 'monitor_closely',
                message: 'Monitor plant health and adjust care routine'
            });
        }
        
        // Disease recommendations
        analysis.diseases.forEach(disease => {
            recommendations.push({
                type: 'disease',
                priority: disease.severity === 'high' ? 'high' : 'medium',
                action: disease.treatment,
                message: `Detected ${disease.type.replace('_', ' ')} - apply ${disease.treatment.replace('_', ' ')}`
            });
        });
        
        // Pest recommendations
        analysis.pests.forEach(pest => {
            recommendations.push({
                type: 'pest',
                priority: pest.severity === 'high' ? 'high' : 'medium',
                action: 'pest_control',
                message: `Detected ${pest.type.replace('_', ' ')} - apply appropriate pest control`
            });
        });
        
        // Growth recommendations
        if (analysis.growth.isStunted) {
            recommendations.push({
                type: 'growth',
                priority: 'medium',
                action: 'improve_conditions',
                message: 'Plant growth is stunted - check lighting and nutrients'
            });
        }
        
        return recommendations;
    }
    
    updateGameStateWithAnalysis(analysisResults) {
        // Update plant health in game state
        analysisResults.forEach(result => {
            const plant = this.game.gameState.plants.find(p => p.id === result.plantId);
            if (plant) {
                plant.health = result.health;
                plant.lastAnalysis = result;
            }
        });
        
        // Update UI with analysis results
        this.updateAnalysisUI(analysisResults);
    }
    
    updateAnalysisUI(analysisResults) {
        if (analysisResults.length === 0) return;
        
        const overallHealth = this.calculateOverallHealth(analysisResults);
        const healthIndicator = this.getHealthIndicator(overallHealth);
        
        // Update health display
        const healthFill = document.getElementById('health-fill');
        const healthScore = document.getElementById('health-score');
        
        if (healthFill && healthScore) {
            const percentage = Math.round(overallHealth * 100);
            healthFill.style.width = `${percentage}%`;
            healthFill.style.backgroundColor = healthIndicator.color;
            healthScore.textContent = `${percentage}%`;
        }
        
        // Update recommendations
        const allRecommendations = analysisResults.flatMap(result => result.recommendations);
        const topRecommendations = allRecommendations
            .sort((a, b) => {
                const priorityOrder = { high: 3, medium: 2, low: 1 };
                return priorityOrder[b.priority] - priorityOrder[a.priority];
            })
            .slice(0, 5);
        
        this.game.uiManager?.updateRecommendations(
            topRecommendations.map(rec => rec.message)
        );
    }
    
    calculateOverallHealth(analysisResults) {
        if (analysisResults.length === 0) return 0.8;
        
        const totalHealth = analysisResults.reduce((sum, result) => sum + result.health, 0);
        return totalHealth / analysisResults.length;
    }
    
    getHealthIndicator(health) {
        for (const [level, indicator] of Object.entries(this.healthIndicators)) {
            if (health >= indicator.min) {
                return indicator;
            }
        }
        return this.healthIndicators.critical;
    }
    
    performDeepAnalysis() {
        console.log('🔬 Performing deep plant analysis...');
        
        // Simulate deep learning analysis
        const deepAnalysis = {
            timestamp: Date.now(),
            geneticAnalysis: this.performGeneticAnalysis(),
            environmentalImpact: this.analyzeEnvironmentalImpact(),
            predictiveModeling: this.generatePredictions(),
            optimizationSuggestions: this.generateOptimizationSuggestions()
        };
        
        // Store deep analysis results
        this.lastDeepAnalysis = deepAnalysis;
        
        // Notify user of insights
        if (deepAnalysis.optimizationSuggestions.length > 0) {
            this.game.notificationManager.show(
                'Deep Analysis Complete 🔬',
                `Found ${deepAnalysis.optimizationSuggestions.length} optimization opportunities`,
                'info'
            );
        }
    }
    
    performGeneticAnalysis() {
        // Simulate genetic analysis of plants
        return {
            averageGenetics: {
                growthRate: 0.8 + Math.random() * 0.4,
                diseaseResistance: 0.6 + Math.random() * 0.3,
                yieldPotential: 0.7 + Math.random() * 0.3
            },
            recommendations: [
                'Consider selective breeding for disease resistance',
                'Optimize genetics for current environment'
            ]
        };
    }
    
    analyzeEnvironmentalImpact() {
        // Analyze how environment affects plant health
        const envParams = this.game.aiEnvironments?.currentParameters || {};
        
        return {
            temperatureImpact: this.calculateTemperatureImpact(envParams.temperature),
            humidityImpact: this.calculateHumidityImpact(envParams.humidity),
            lightingImpact: this.calculateLightingImpact(envParams.lightIntensity),
            co2Impact: this.calculateCO2Impact(envParams.co2Level)
        };
    }
    
    calculateTemperatureImpact(temperature) {
        const optimal = 75;
        const deviation = Math.abs(temperature - optimal);
        return Math.max(0, 1 - deviation / 20);
    }
    
    calculateHumidityImpact(humidity) {
        const optimal = 50;
        const deviation = Math.abs(humidity - optimal);
        return Math.max(0, 1 - deviation / 30);
    }
    
    calculateLightingImpact(lightIntensity) {
        const optimal = 80;
        const deviation = Math.abs(lightIntensity - optimal);
        return Math.max(0, 1 - deviation / 40);
    }
    
    calculateCO2Impact(co2Level) {
        if (co2Level < 400) return 0.6;
        if (co2Level > 1500) return 0.7;
        return Math.min(1, co2Level / 1000);
    }
    
    generatePredictions() {
        // Generate predictions for plant development
        return {
            harvestDate: this.predictHarvestDate(),
            expectedYield: this.predictYield(),
            potentialIssues: this.predictPotentialIssues()
        };
    }
    
    predictHarvestDate() {
        const plants = this.game.gameState.plants;
        const floweringPlants = plants.filter(p => p.stage === 'flowering');
        
        if (floweringPlants.length === 0) return null;
        
        const avgFloweringTime = 56; // days
        const avgAge = floweringPlants.reduce((sum, p) => sum + p.age, 0) / floweringPlants.length;
        const daysRemaining = Math.max(0, avgFloweringTime - avgAge);
        
        const harvestDate = new Date();
        harvestDate.setDate(harvestDate.getDate() + daysRemaining);
        
        return harvestDate;
    }
    
    predictYield() {
        const plants = this.game.gameState.plants;
        const healthyPlants = plants.filter(p => p.health > 0.7);
        
        return {
            estimatedYield: healthyPlants.length * 50, // grams per plant
            qualityScore: this.calculateOverallHealth(this.analysisHistory.slice(-1)[0]?.results || [])
        };
    }
    
    predictPotentialIssues() {
        const issues = [];
        
        // Predict based on current trends
        if (this.analysisHistory.length > 5) {
            const recentHealth = this.analysisHistory.slice(-5).map(h => h.overallHealth);
            const healthTrend = recentHealth[recentHealth.length - 1] - recentHealth[0];
            
            if (healthTrend < -0.1) {
                issues.push({
                    type: 'declining_health',
                    probability: 0.7,
                    timeframe: '3-5 days',
                    prevention: 'Adjust environment parameters'
                });
            }
        }
        
        return issues;
    }
    
    generateOptimizationSuggestions() {
        const suggestions = [];
        
        // Analyze current performance and suggest improvements
        const currentHealth = this.calculateOverallHealth(
            this.analysisHistory.slice(-1)[0]?.results || []
        );
        
        if (currentHealth < 0.9) {
            suggestions.push({
                type: 'environment_optimization',
                impact: 'high',
                description: 'Optimize environmental parameters for better plant health',
                expectedImprovement: '10-15% health increase'
            });
        }
        
        return suggestions;
    }
    
    updateHealthMetrics() {
        // Update real-time health metrics
        const plants = this.game.gameState.plants;
        
        if (plants.length > 0) {
            const avgHealth = plants.reduce((sum, p) => sum + p.health, 0) / plants.length;
            
            this.healthMetrics.overallHealth = avgHealth;
            this.healthMetrics.leafColor = avgHealth * 0.9 + Math.random() * 0.1;
            this.healthMetrics.stemThickness = avgHealth * 0.8 + Math.random() * 0.2;
            this.healthMetrics.growthRate = avgHealth * 0.7 + Math.random() * 0.3;
        }
    }
    
    getHealthMetrics() {
        return { ...this.healthMetrics };
    }
    
    getAnalysisHistory() {
        return [...this.analysisHistory];
    }
    
    getLastDeepAnalysis() {
        return this.lastDeepAnalysis;
    }
}

// Smart Room AI
class SmartRoomAI {
    constructor(game) {
        this.game = game;
        this.automationRules = [];
        this.learningData = [];
        this.isLearning = true;
        this.automationEnabled = true;
        
        // Automation presets
        this.presets = {
            beginner: {
                name: 'Beginner Friendly',
                description: 'Conservative automation for new growers',
                rules: [
                    { trigger: 'low_health', action: 'increase_humidity', threshold: 0.6 },
                    { trigger: 'high_temperature', action: 'increase_ventilation', threshold: 80 },
                    { trigger: 'low_co2', action: 'increase_co2', threshold: 350 }
                ]
            },
            advanced: {
                name: 'Advanced Optimization',
                description: 'Aggressive optimization for experienced growers',
                rules: [
                    { trigger: 'growth_optimization', action: 'dynamic_adjustment', threshold: 0.8 },
                    { trigger: 'efficiency_mode', action: 'resource_optimization', threshold: 0.9 },
                    { trigger: 'predictive_adjustment', action: 'preemptive_care', threshold: 0.7 }
                ]
            },
            custom: {
                name: 'Custom Rules',
                description: 'User-defined automation rules',
                rules: []
            }
        };
        
        this.currentPreset = 'beginner';
        this.init();
    }
    
    async init() {
        console.log('🤖 Initializing Smart Room AI...');
        
        try {
            // Load automation rules
            this.loadAutomationRules();
            
            // Start automation loop
            this.startAutomationLoop();
            
            // Initialize learning system
            this.initializeLearning();
            
            console.log('✅ Smart Room AI initialized successfully');
            
        } catch (error) {
            console.error('❌ Failed to initialize Smart Room AI:', error);
        }
    }
    
    loadAutomationRules() {
        this.automationRules = [...this.presets[this.currentPreset].rules];
        console.log(`📋 Loaded ${this.automationRules.length} automation rules`);
    }
    
    startAutomationLoop() {
        // Run automation checks every 60 seconds
        setInterval(() => {
            if (this.automationEnabled) {
                this.runAutomation();
            }
        }, 60000);
        
        // Learn from user actions every 5 minutes
        setInterval(() => {
            if (this.isLearning) {
                this.learnFromUserActions();
            }
        }, 5 * 60 * 1000);
    }
    
    initializeLearning() {
        // Initialize machine learning components
        this.learningModel = {
            userPreferences: {},
            actionPatterns: [],
            successMetrics: [],
            adaptationRate: 0.1
        };
    }
    
    runAutomation() {
        console.log('🤖 Running smart room automation...');
        
        const currentState = this.getCurrentRoomState();
        const actions = [];
        
        // Check each automation rule
        this.automationRules.forEach(rule => {
            const shouldTrigger = this.evaluateRule(rule, currentState);
            if (shouldTrigger) {
                const action = this.generateAction(rule, currentState);
                if (action) {
                    actions.push(action);
                }
            }
        });
        
        // Execute actions
        actions.forEach(action => {
            this.executeAction(action);
        });
        
        // Learn from automation results
        if (actions.length > 0) {
            this.recordAutomationResults(actions, currentState);
        }
    }
    
    getCurrentRoomState() {
        const plants = this.game.gameState.plants;
        const envParams = this.game.aiEnvironments?.currentParameters || {};
        const healthMetrics = this.game.cvPlantEngine?.getHealthMetrics() || {};
        
        return {
            plantCount: plants.length,
            averageHealth: plants.length > 0 ? 
                plants.reduce((sum, p) => sum + p.health, 0) / plants.length : 0,
            environment: {
                temperature: envParams.temperature || 72,
                humidity: envParams.humidity || 50,
                co2Level: envParams.co2Level || 400,
                lightIntensity: envParams.lightIntensity || 75
            },
            healthMetrics,
            timestamp: Date.now()
        };
    }
    
    evaluateRule(rule, state) {
        switch (rule.trigger) {
            case 'low_health':
                return state.averageHealth < rule.threshold;
            case 'high_temperature':
                return state.environment.temperature > rule.threshold;
            case 'low_co2':
                return state.environment.co2Level < rule.threshold;
            case 'growth_optimization':
                return state.averageHealth > rule.threshold && this.shouldOptimizeGrowth(state);
            case 'efficiency_mode':
                return state.averageHealth > rule.threshold && this.shouldOptimizeEfficiency(state);
            case 'predictive_adjustment':
                return this.shouldMakePredictiveAdjustment(state, rule.threshold);
            default:
                return false;
        }
    }
    
    shouldOptimizeGrowth(state) {
        // Check if conditions are good for growth optimization
        return state.plantCount > 0 && 
               state.environment.temperature > 70 && 
               state.environment.temperature < 80;
    }
    
    shouldOptimizeEfficiency(state) {
        // Check if we can reduce resource usage while maintaining health
        return state.averageHealth > 0.9 && 
               state.environment.co2Level > 800;
    }
    
    shouldMakePredictiveAdjustment(state, threshold) {
        // Use historical data to predict if adjustment is needed
        const recentData = this.learningData.slice(-5);
        if (recentData.length < 3) return false;
        
        const healthTrend = this.calculateHealthTrend(recentData);
        return healthTrend < threshold;
    }
    
    calculateHealthTrend(data) {
        if (data.length < 2) return 0;
        
        const first = data[0].averageHealth;
        const last = data[data.length - 1].averageHealth;
        return (last - first) / data.length;
    }
    
    generateAction(rule, state) {
        switch (rule.action) {
            case 'increase_humidity':
                return {
                    type: 'environment_adjustment',
                    parameter: 'humidity',
                    value: Math.min(70, state.environment.humidity + 5),
                    reason: 'Low plant health detected'
                };
            case 'increase_ventilation':
                return {
                    type: 'environment_adjustment',
                    parameter: 'temperature',
                    value: Math.max(65, state.environment.temperature - 2),
                    reason: 'High temperature detected'
                };
            case 'increase_co2':
                return {
                    type: 'environment_adjustment',
                    parameter: 'co2Level',
                    value: Math.min(1200, state.environment.co2Level + 100),
                    reason: 'Low CO₂ levels detected'
                };
            case 'dynamic_adjustment':
                return this.generateDynamicAdjustment(state);
            case 'resource_optimization':
                return this.generateResourceOptimization(state);
            case 'preemptive_care':
                return this.generatePreemptiveCare(state);
            default:
                return null;
        }
    }
    
    generateDynamicAdjustment(state) {
        // Generate intelligent adjustment based on current conditions
        const adjustments = [];
        
        // Optimize temperature
        const optimalTemp = 75;
        if (Math.abs(state.environment.temperature - optimalTemp) > 2) {
            adjustments.push({
                type: 'environment_adjustment',
                parameter: 'temperature',
                value: optimalTemp,
                reason: 'Dynamic temperature optimization'
            });
        }
        
        // Optimize humidity based on growth stage
        const plants = this.game.gameState.plants;
        const floweringPlants = plants.filter(p => p.stage === 'flowering');
        const optimalHumidity = floweringPlants.length > 0 ? 45 : 55;
        
        if (Math.abs(state.environment.humidity - optimalHumidity) > 5) {
            adjustments.push({
                type: 'environment_adjustment',
                parameter: 'humidity',
                value: optimalHumidity,
                reason: 'Dynamic humidity optimization for growth stage'
            });
        }
        
        return adjustments.length > 0 ? adjustments[0] : null;
    }
    
    generateResourceOptimization(state) {
        // Optimize resource usage while maintaining plant health
        return {
            type: 'environment_adjustment',
            parameter: 'co2Level',
            value: Math.max(600, state.environment.co2Level - 100),
            reason: 'Resource efficiency optimization'
        };
    }
    
    generatePreemptiveCare(state) {
        // Make preemptive adjustments based on predictions
        const predictions = this.game.cvPlantEngine?.getLastDeepAnalysis()?.predictiveModeling;
        
        if (predictions?.potentialIssues?.length > 0) {
            const issue = predictions.potentialIssues[0];
            
            switch (issue.type) {
                case 'declining_health':
                    return {
                        type: 'environment_adjustment',
                        parameter: 'humidity',
                        value: Math.min(65, state.environment.humidity + 3),
                        reason: 'Preemptive care for predicted health decline'
                    };
                default:
                    return null;
            }
        }
        
        return null;
    }
    
    executeAction(action) {
        console.log(`🤖 Executing automation action: ${action.type}`);
        
        if (action.type === 'environment_adjustment') {
            // Apply the environmental adjustment
            this.game.aiEnvironments?.updateEnvironmentParameter(
                action.parameter.replace(/([A-Z])/g, '-$1').toLowerCase(),
                action.value
            );
            
            // Notify user
            this.game.notificationManager.show(
                'Smart Room Automation 🤖',
                `${action.reason}: Adjusted ${action.parameter} to ${action.value}`,
                'info'
            );
        }
        
        // Record action for learning
        this.recordAction(action);
    }
    
    recordAction(action) {
        this.learningData.push({
            timestamp: Date.now(),
            action: action,
            stateBefore: this.getCurrentRoomState(),
            source: 'automation'
        });
        
        // Limit learning data size
        if (this.learningData.length > 1000) {
            this.learningData.shift();
        }
    }
    
    recordAutomationResults(actions, initialState) {
        // Record the results of automation for learning
        setTimeout(() => {
            const finalState = this.getCurrentRoomState();
            const improvement = finalState.averageHealth - initialState.averageHealth;
            
            actions.forEach(action => {
                this.learningModel.successMetrics.push({
                    action: action,
                    improvement: improvement,
                    timestamp: Date.now()
                });
            });
        }, 30000); // Check results after 30 seconds
    }
    
    learnFromUserActions() {
        console.log('🧠 Learning from user actions...');
        
        // Analyze user action patterns
        const userActions = this.learningData.filter(data => data.source === 'user');
        const recentUserActions = userActions.slice(-20);
        
        if (recentUserActions.length > 5) {
            // Identify user preferences
            this.identifyUserPreferences(recentUserActions);
            
            // Adapt automation rules
            this.adaptAutomationRules(recentUserActions);
            
            // Update learning model
            this.updateLearningModel(recentUserActions);
        }
    }
    
    identifyUserPreferences(userActions) {
        // Analyze user's preferred parameter ranges
        const preferences = {};
        
        userActions.forEach(action => {
            if (action.action.type === 'environment_adjustment') {
                const param = action.action.parameter;
                if (!preferences[param]) {
                    preferences[param] = [];
                }
                preferences[param].push(action.action.value);
            }
        });
        
        // Calculate preferred ranges
        Object.keys(preferences).forEach(param => {
            const values = preferences[param];
            const avg = values.reduce((sum, val) => sum + val, 0) / values.length;
            const min = Math.min(...values);
            const max = Math.max(...values);
            
            this.learningModel.userPreferences[param] = {
                preferred: avg,
                range: { min, max },
                confidence: Math.min(1, values.length / 10)
            };
        });
    }
    
    adaptAutomationRules(userActions) {
        // Adapt automation rules based on user behavior
        const userOverrides = userActions.filter(action => 
            this.wasAutomationOverridden(action)
        );
        
        if (userOverrides.length > 3) {
            console.log('🔄 Adapting automation rules based on user feedback');
            
            // Adjust rule thresholds
            this.automationRules.forEach(rule => {
                const relevantOverrides = userOverrides.filter(override => 
                    this.isRuleRelevant(rule, override)
                );
                
                if (relevantOverrides.length > 0) {
                    // Adjust threshold based on user preferences
                    rule.threshold = this.calculateAdjustedThreshold(rule, relevantOverrides);
                }
            });
        }
    }
    
    wasAutomationOverridden(action) {
        // Check if user action was made shortly after automation
        const automationActions = this.learningData.filter(data => 
            data.source === 'automation' && 
            Math.abs(data.timestamp - action.timestamp) < 300000 // 5 minutes
        );
        
        return automationActions.length > 0;
    }
    
    isRuleRelevant(rule, userAction) {
        // Check if user action is relevant to the automation rule
        if (userAction.action.type !== 'environment_adjustment') return false;
        
        const actionParam = userAction.action.parameter;
        
        switch (rule.action) {
            case 'increase_humidity':
                return actionParam === 'humidity';
            case 'increase_ventilation':
                return actionParam === 'temperature';
            case 'increase_co2':
                return actionParam === 'co2Level';
            default:
                return false;
        }
    }
    
    calculateAdjustedThreshold(rule, userOverrides) {
        // Calculate new threshold based on user behavior
        const userValues = userOverrides.map(override => override.action.value);
        const avgUserValue = userValues.reduce((sum, val) => sum + val, 0) / userValues.length;
        
        // Adjust threshold towards user preference
        const adjustment = (avgUserValue - rule.threshold) * this.learningModel.adaptationRate;
        return rule.threshold + adjustment;
    }
    
    updateLearningModel(userActions) {
        // Update the learning model with new data
        this.learningModel.actionPatterns = this.analyzeActionPatterns(userActions);
        
        // Increase adaptation rate if user frequently overrides automation
        const overrideRate = userActions.filter(action => 
            this.wasAutomationOverridden(action)
        ).length / userActions.length;
        
        if (overrideRate > 0.3) {
            this.learningModel.adaptationRate = Math.min(0.3, this.learningModel.adaptationRate * 1.1);
        } else {
            this.learningModel.adaptationRate = Math.max(0.05, this.learningModel.adaptationRate * 0.95);
        }
    }
    
    analyzeActionPatterns(userActions) {
        // Analyze patterns in user actions
        const patterns = {};
        
        userActions.forEach(action => {
            const hour = new Date(action.timestamp).getHours();
            const actionType = action.action.type;
            
            if (!patterns[hour]) {
                patterns[hour] = {};
            }
            
            if (!patterns[hour][actionType]) {
                patterns[hour][actionType] = 0;
            }
            
            patterns[hour][actionType]++;
        });
        
        return patterns;
    }
    
    // Public methods for user interaction
    setAutomationEnabled(enabled) {
        this.automationEnabled = enabled;
        console.log(`🤖 Automation ${enabled ? 'enabled' : 'disabled'}`);
        
        this.game.notificationManager.show(
            'Automation Settings',
            `Smart room automation ${enabled ? 'enabled' : 'disabled'}`,
            'info'
        );
    }
    
    setLearningEnabled(enabled) {
        this.isLearning = enabled;
        console.log(`🧠 Learning ${enabled ? 'enabled' : 'disabled'}`);
    }
    
    changePreset(presetName) {
        if (this.presets[presetName]) {
            this.currentPreset = presetName;
            this.loadAutomationRules();
            
            this.game.notificationManager.show(
                'Automation Preset Changed',
                `Switched to ${this.presets[presetName].name}`,
                'info'
            );
        }
    }
    
    addCustomRule(rule) {
        this.automationRules.push(rule);
        console.log('📋 Added custom automation rule');
    }
    
    getAutomationStatus() {
        return {
            enabled: this.automationEnabled,
            learning: this.isLearning,
            preset: this.currentPreset,
            rulesCount: this.automationRules.length,
            learningData: this.learningData.length,
            userPreferences: this.learningModel.userPreferences
        };
    }
    
    exportLearningData() {
        return {
            learningModel: this.learningModel,
            automationRules: this.automationRules,
            learningData: this.learningData.slice(-100) // Last 100 entries
        };
    }
}

// Export classes for use in main game
window.CVPlantEngine = CVPlantEngine;
window.SmartRoomAI = SmartRoomAI;

