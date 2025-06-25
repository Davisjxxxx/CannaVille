# CannaVille Pro 🌿

A hyper-realistic 3D cannabis cultivation game with AI-powered features, smart room automation, and diverse avatar system.

## Features

### 🎮 Game Features
- **Hyper-realistic 3D environments** - Indoor, outdoor, and greenhouse growing spaces
- **Diverse avatar system** - 8 unique characters representing different backgrounds and expertise
- **Interactive plant cultivation** - Full growth cycle from seedling to harvest
- **Smart room automation** - AI-powered environmental control and optimization

### 🤖 AI Features
- **AI texture generation** - Create custom textures for plants, equipment, and environments
- **Computer vision plant analysis** - Real-time health monitoring and disease detection
- **Predictive modeling** - Forecast plant development and potential issues
- **Smart automation** - Learn from user behavior and optimize growing conditions

### 🏠 Smart Room Features
- **Environmental monitoring** - Temperature, humidity, CO₂, light, and airflow sensors
- **Automated adjustments** - Intelligent parameter optimization
- **Energy efficiency** - Resource usage optimization while maintaining plant health
- **Performance analytics** - Detailed metrics and improvement suggestions

## Quick Start

### Prerequisites
- Node.js 16 or higher
- Python 3.7 or higher
- Modern web browser with WebGL support

### Installation

1. **Clone or extract the project**
   ```bash
   cd CannaVille
   ```

2. **Run the deployment script**
   ```bash
   ./deploy.sh
   ```

3. **Start the application**
   ```bash
   ./start.sh
   ```

4. **Open your browser**
   - Web interface: http://localhost:8080
   - API documentation: http://localhost:5000/api/health

### Development Mode

For development with debug features:
```bash
./start-dev.sh
```

## Avatar System

Choose from 8 diverse characters, each with unique backgrounds and specialties:

- **Jake Thompson** - Experienced outdoor grower from Colorado
- **Sarah Mitchell** - Hydroponic specialist and botanist  
- **Marcus Johnson** - Organic cultivation expert from California
- **Amara Williams** - Sustainable farming advocate and educator
- **Hiroshi Tanaka** - Precision agriculture technologist
- **Li Wei Chen** - Traditional medicine and cannabis researcher
- **Carlos Rodriguez** - Multi-generational farmer with family traditions
- **Isabella Martinez** - Cannabis entrepreneur and business owner

## AI Texture Generation

Generate custom textures using AI:

1. Open the AI panel in the game
2. Enter a descriptive prompt
3. Select style and resolution
4. Generate and apply to your environment

### Example Prompts
- "Dense cannabis buds with crystalline trichomes"
- "Organic soil texture with rich nutrients"
- "LED grow light panel with full spectrum"
- "Hydroponic growing medium texture"

## Smart Room Automation

The AI system automatically:
- Monitors environmental conditions
- Adjusts parameters for optimal growth
- Learns from your preferences
- Provides efficiency recommendations
- Predicts and prevents issues

### Automation Levels
- **Low** - Manual control with basic alerts
- **Medium** - Automated adjustments with user oversight
- **High** - Full automation with predictive optimization

## API Endpoints

### Texture Generation
```
POST /api/generate-texture
{
  "prompt": "texture description",
  "style": "photorealistic",
  "resolution": 1024,
  "category": "cannabis"
}
```

### Plant Analysis
```
POST /api/analyze-plant
{
  "id": "plant_id",
  "health": 0.8,
  "stage": "flowering"
}
```

### Health Check
```
GET /api/health
```

## Configuration

Edit `config.json` to customize:
- Feature toggles
- Performance settings
- API configuration
- Environment parameters

## Troubleshooting

### Common Issues

1. **Port already in use**
   ```bash
   # Kill existing processes
   pkill -f "python.*5000"
   pkill -f "python.*8080"
   ```

2. **Missing dependencies**
   ```bash
   # Reinstall dependencies
   npm install
   pip3 install -r api/requirements.txt
   ```

3. **WebGL not supported**
   - Update your browser
   - Enable hardware acceleration
   - Check graphics drivers

### Performance Optimization

- Close unnecessary browser tabs
- Reduce texture resolution in settings
- Lower plant count for better performance
- Use Chrome or Firefox for best WebGL support

## Development

### Project Structure
```
CannaVille/
├── index.html              # Main game interface
├── css/                    # Stylesheets
│   ├── main.css           # Main game styles
│   └── ui.css             # UI component styles
├── js/                     # JavaScript modules
│   ├── main.js            # Main game engine
│   ├── avatar-system.js   # Avatar management
│   ├── ai-environments.js # AI environment control
│   ├── plant-generator.js # Plant generation and UI
│   ├── cv-plant-engine.js # Computer vision analysis
│   └── smart-room-ai.js   # Smart room automation
├── api/                    # Backend API
│   ├── ai-texture-generator.py
│   └── start_api.py       # API server
├── assets/                 # Game assets
│   ├── models/            # 3D models
│   ├── textures/          # Texture files
│   └── sounds/            # Audio files
└── dist/                   # Built application
```

### Adding New Features

1. Create new JavaScript modules in `js/`
2. Add API endpoints in `api/start_api.py`
3. Update UI in `index.html` and CSS files
4. Test locally before deployment

## License

This project is for educational and demonstration purposes.

## Support

For issues and questions:
1. Check the troubleshooting section
2. Review browser console for errors
3. Verify all dependencies are installed
4. Ensure ports 5000 and 8080 are available

---

**Enjoy growing in CannaVille Pro! 🌿**
