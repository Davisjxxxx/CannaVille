# CannaVille Pro - Technical Documentation

## Overview

CannaVille Pro is a hyper-realistic 3D cannabis cultivation simulator that provides an immersive, educational, and interactive experience for users to learn about cannabis growing techniques. The application features first-person navigation, realistic plant models, environmental controls, and comprehensive game mechanics.

## Architecture

### Frontend
- **Technology**: HTML5, CSS3, JavaScript (ES6+), Three.js
- **3D Engine**: Three.js with WebGL rendering
- **Controls**: PointerLockControls for first-person movement
- **Asset Loading**: GLTFLoader with DRACO compression support
- **Responsive Design**: Mobile-friendly UI with touch support

### Backend
- **Framework**: Flask (Python)
- **Database**: SQLAlchemy with SQLite
- **Security**: Flask-Security-Too, CORS enabled
- **API**: RESTful endpoints for game state, user data, and payments
- **Payment Processing**: Stripe integration (configured for test mode)

## Features Implemented

### 3D Environment
- **Indoor DWC System**: 6-bucket recirculating deep water culture setup in 4×4 tent
- **Outdoor Farm**: 5×5 grid of hand-dug holes with realistic spacing
- **Hyper-realistic Lighting**: Dynamic lighting with shadows and environmental effects
- **Asset Integration**: Support for GLB models with multiple growth stages

### Interactive Gameplay
- **First-Person Controls**: WASD movement, mouse look, E to interact
- **Plant Lifecycle**: 4 growth stages from seedling to harvest
- **Tool System**: Water, nutrients, neem oil, ladybugs for plant care
- **Environmental Controls**: Light intensity, temperature, humidity sliders
- **Health System**: Plant health monitoring and care mechanics

### User Interface
- **Professional Design**: Modern gaming UI with dark theme
- **Real-time Stats**: Money, energy, level, day tracking
- **Tool Selection**: Visual tool inventory with quantity tracking
- **Environment Switching**: Seamless indoor/outdoor environment toggle
- **Plant Analysis**: Health score display and environmental settings

### Backend API
- **User Management**: Registration, authentication, profile management
- **Game State**: Save/load game progress and statistics
- **Payment Integration**: Stripe checkout for in-app purchases
- **Security**: Secure password hashing, session management

## Technical Specifications

### Performance Optimizations
- **LOD System**: Level-of-detail rendering for complex models
- **Asset Compression**: DRACO geometry compression
- **Efficient Rendering**: Frustum culling, shadow optimization
- **Mobile Support**: Responsive design with touch controls

### Security Features
- **Data Protection**: Encrypted password storage
- **CORS Configuration**: Secure cross-origin requests
- **Input Validation**: Server-side validation for all API endpoints
- **Session Management**: Secure user session handling

### Mobile Deployment Ready
- **Responsive UI**: Adapts to various screen sizes
- **Touch Controls**: Mobile-friendly interaction system
- **Performance Scaling**: Automatic quality adjustment for mobile devices
- **PWA Support**: Progressive Web App capabilities

## File Structure

```
CannaVillePro_Backend/
├── src/
│   ├── static/
│   │   ├── index.html              # Main application entry point
│   │   ├── js/
│   │   │   └── cannaville-pro.js   # Main game engine
│   │   ├── css/
│   │   │   └── styles.css          # Application styling
│   │   └── assets/
│   │       └── models/
│   │           └── plants/         # GLB plant models
│   ├── models/
│   │   ├── user.py                 # User database model
│   │   └── game_data.py            # Game state model
│   ├── routes/
│   │   ├── user.py                 # User API endpoints
│   │   ├── game.py                 # Game API endpoints
│   │   └── stripe.py               # Payment API endpoints
│   └── main.py                     # Flask application entry point
├── requirements.txt                # Python dependencies
└── README.md                       # Project documentation
```

## API Endpoints

### Game Management
- `POST /api/game/save` - Save game state
- `GET /api/game/load/<user_id>` - Load game state

### User Management
- `POST /api/users` - Create new user
- `GET /api/users/<user_id>` - Get user profile
- `PUT /api/users/<user_id>` - Update user profile

### Payment Processing
- `GET /api/stripe/config` - Get Stripe configuration
- `POST /api/stripe/create-checkout-session` - Create payment session

## Deployment

### Production URL
**Live Application**: https://g8h3ilc7g3em.manus.space

### Deployment Features
- **Automatic Scaling**: Cloud-based deployment with auto-scaling
- **SSL Security**: HTTPS encryption for all communications
- **CDN Integration**: Fast global content delivery
- **Database Backup**: Automated backup and recovery

## Browser Compatibility

### Supported Browsers
- **Chrome**: 80+ (Recommended)
- **Firefox**: 75+
- **Safari**: 13+
- **Edge**: 80+

### Required Features
- WebGL 2.0 support
- Pointer Lock API
- ES6+ JavaScript support
- Local Storage API

## Performance Metrics

### Target Performance
- **Frame Rate**: 60 FPS on desktop, 30 FPS on mobile
- **Load Time**: < 5 seconds initial load
- **Asset Size**: < 25MB total payload
- **Memory Usage**: < 512MB RAM

### Optimization Techniques
- **Asset Compression**: 60-80% size reduction
- **Texture Optimization**: KTX2 format support
- **Geometry Simplification**: LOD-based rendering
- **Shader Optimization**: Efficient material rendering

## Future Enhancements

### Planned Features
- **Multiplayer Support**: Real-time collaboration
- **Advanced AI**: Machine learning plant optimization
- **VR Support**: Virtual reality integration
- **Marketplace**: User-generated content trading
- **Social Features**: Community sharing and competitions

### Technical Roadmap
- **Database Migration**: PostgreSQL for production scaling
- **Microservices**: Service-oriented architecture
- **Real-time Updates**: WebSocket integration
- **Advanced Analytics**: Player behavior tracking

## Troubleshooting

### Common Issues
1. **Loading Screen Stuck**: Clear browser cache and reload
2. **3D Not Rendering**: Ensure WebGL is enabled in browser
3. **Controls Not Working**: Click in 3D viewport to lock mouse
4. **Performance Issues**: Lower graphics quality in settings

### Debug Information
- **Console Logs**: Check browser developer tools
- **Network Issues**: Verify internet connection
- **Browser Support**: Update to latest browser version
- **Hardware Requirements**: Ensure GPU acceleration is enabled

## Support and Maintenance

### Monitoring
- **Error Tracking**: Automatic error reporting
- **Performance Monitoring**: Real-time performance metrics
- **User Analytics**: Usage pattern analysis
- **Security Scanning**: Regular vulnerability assessments

### Update Process
- **Continuous Deployment**: Automated testing and deployment
- **Version Control**: Git-based source control
- **Rollback Capability**: Quick reversion to previous versions
- **Feature Flags**: Gradual feature rollout

---

*This documentation covers the current implementation of CannaVille Pro. For additional technical details or support, please refer to the source code comments and API documentation.*

