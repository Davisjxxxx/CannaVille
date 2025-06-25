# CannaVille Pro - User Guide & Deployment Instructions

## User Guide

### Getting Started

#### System Requirements
- **Browser**: Chrome 80+, Firefox 75+, Safari 13+, or Edge 80+
- **Hardware**: GPU with WebGL 2.0 support
- **Memory**: 4GB RAM minimum, 8GB recommended
- **Internet**: Stable broadband connection

#### First Launch
1. Navigate to: https://g8h3ilc7g3em.manus.space
2. Wait for the loading screen to complete
3. Click anywhere in the 3D viewport to lock mouse controls
4. Use WASD keys to move around the environment

### Controls

#### Movement
- **W, A, S, D**: Move forward, left, backward, right
- **Mouse**: Look around (first-person view)
- **Space**: Jump
- **E**: Interact with nearby plants
- **Click**: Lock/unlock mouse controls

#### Interface
- **Tool Selection**: Click on tool buttons in the left panel
- **Environment Switch**: Toggle between Indoor DWC and Outdoor Farm
- **Environmental Controls**: Adjust light, temperature, and humidity sliders
- **Plant Analysis**: View plant health and statistics in the right panel

### Gameplay Mechanics

#### Plant Care
1. **Watering**: Select water tool and interact with plants
2. **Nutrients**: Use nutrient tool to feed plants for better growth
3. **Pest Control**: Apply neem oil or release ladybugs for pest management
4. **Inspection**: Use inspect tool to check plant health and growth stage

#### Environments
- **Indoor DWC**: 6-bucket hydroponic system in grow tent
- **Outdoor Farm**: 5×5 grid of soil holes for traditional growing

#### Progression
- **Money**: Earn by harvesting mature plants
- **Energy**: Depletes with activities, regenerates over time
- **Level**: Increases with successful harvests and plant care
- **Day Cycle**: Game progresses through daily cycles

### Tips for Success
1. **Regular Care**: Check plants daily for optimal health
2. **Environmental Control**: Maintain proper light, temperature, and humidity
3. **Pest Management**: Address pest issues quickly to prevent spread
4. **Harvest Timing**: Wait for plants to reach full maturity for maximum yield
5. **Resource Management**: Balance spending on tools and supplies

## Deployment Instructions

### For Developers

#### Local Development Setup

##### Prerequisites
```bash
# Install Python 3.8+
python --version

# Install Node.js 16+ (for asset processing)
node --version
```

##### Backend Setup
```bash
# Clone or download the project
cd CannaVillePro_Backend

# Create virtual environment
python -m venv venv

# Activate virtual environment
# On Windows:
venv\Scripts\activate
# On macOS/Linux:
source venv/bin/activate

# Install dependencies
pip install -r requirements.txt

# Run the application
python src/main.py
```

##### Frontend Development
```bash
# Serve static files for development
cd src/static
python -m http.server 8080

# Or use the Flask development server
python src/main.py
```

#### Production Deployment

##### Using Manus Platform
```bash
# Deploy to Manus (recommended)
manus-deploy-backend flask CannaVillePro_Backend

# The platform will automatically:
# - Install dependencies
# - Configure SSL
# - Set up auto-scaling
# - Provide permanent URL
```

##### Manual Deployment Options

###### Docker Deployment
```dockerfile
# Dockerfile
FROM python:3.9-slim

WORKDIR /app
COPY requirements.txt .
RUN pip install -r requirements.txt

COPY src/ ./src/
EXPOSE 5000

CMD ["python", "src/main.py"]
```

```bash
# Build and run
docker build -t cannaville-pro .
docker run -p 5000:5000 cannaville-pro
```

###### Traditional Server Deployment
```bash
# Install dependencies
pip install -r requirements.txt gunicorn

# Run with Gunicorn
gunicorn --bind 0.0.0.0:5000 src.main:app

# Or with uWSGI
uwsgi --http 0.0.0.0:5000 --module src.main:app
```

### For System Administrators

#### Server Requirements
- **CPU**: 2+ cores
- **RAM**: 4GB minimum, 8GB recommended
- **Storage**: 10GB available space
- **Network**: Public IP with ports 80/443 open
- **OS**: Ubuntu 20.04+ or equivalent

#### Security Configuration
```bash
# Enable firewall
ufw enable
ufw allow 80
ufw allow 443
ufw allow 22

# SSL Certificate (using Let's Encrypt)
certbot --nginx -d yourdomain.com

# Database security
# - Use strong passwords
# - Enable encryption at rest
# - Regular backups
```

#### Monitoring Setup
```bash
# Install monitoring tools
pip install prometheus-flask-exporter

# Configure logging
# - Application logs: /var/log/cannaville/
# - Error tracking: Sentry integration
# - Performance monitoring: New Relic/DataDog
```

### Mobile Deployment

#### Progressive Web App (PWA)
The application is PWA-ready and can be installed on mobile devices:

1. **Android**: Open in Chrome, tap "Add to Home Screen"
2. **iOS**: Open in Safari, tap Share → "Add to Home Screen"

#### Native App Deployment (Future)
For native mobile apps, consider:
- **React Native**: Port the frontend to React Native
- **Cordova/PhoneGap**: Wrap the web app in native container
- **Capacitor**: Modern hybrid app development

### Environment Variables

#### Required Configuration
```bash
# Production settings
export FLASK_ENV=production
export SECRET_KEY=your-secret-key-here
export DATABASE_URL=your-database-url

# Stripe configuration
export STRIPE_SECRET_KEY=sk_live_...
export STRIPE_PUBLISHABLE_KEY=pk_live_...

# Optional settings
export DEBUG=False
export LOG_LEVEL=INFO
```

### Database Setup

#### SQLite (Development)
```python
# Automatic setup - no configuration needed
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///app.db'
```

#### PostgreSQL (Production)
```bash
# Install PostgreSQL
sudo apt install postgresql postgresql-contrib

# Create database
sudo -u postgres createdb cannaville_pro

# Update configuration
export DATABASE_URL=postgresql://user:pass@localhost/cannaville_pro
```

### Backup and Recovery

#### Database Backup
```bash
# SQLite backup
cp src/database/app.db backup/app_$(date +%Y%m%d).db

# PostgreSQL backup
pg_dump cannaville_pro > backup/cannaville_$(date +%Y%m%d).sql
```

#### Asset Backup
```bash
# Backup user-generated content
tar -czf backup/assets_$(date +%Y%m%d).tar.gz src/static/assets/
```

### Performance Optimization

#### Frontend Optimization
- **Asset Compression**: Enable gzip/brotli compression
- **CDN Integration**: Use CloudFlare or similar CDN
- **Caching**: Implement browser and server-side caching
- **Image Optimization**: Compress textures and models

#### Backend Optimization
- **Database Indexing**: Add indexes for frequently queried fields
- **Connection Pooling**: Use connection pooling for database
- **Caching**: Implement Redis for session and data caching
- **Load Balancing**: Use multiple server instances for high traffic

### Troubleshooting

#### Common Deployment Issues
1. **Port Conflicts**: Ensure port 5000 is available
2. **Permission Errors**: Check file permissions and ownership
3. **Database Connection**: Verify database credentials and connectivity
4. **SSL Issues**: Ensure certificates are properly configured

#### Debug Commands
```bash
# Check application status
curl -I http://localhost:5000/

# View application logs
tail -f /var/log/cannaville/app.log

# Test database connection
python -c "from src.models.user import db; print('DB OK')"

# Check system resources
htop
df -h
```

### Support and Maintenance

#### Regular Maintenance Tasks
- **Security Updates**: Apply OS and dependency updates monthly
- **Database Cleanup**: Remove old session data and logs
- **Performance Monitoring**: Review metrics and optimize bottlenecks
- **Backup Verification**: Test backup restoration procedures

#### Getting Help
- **Documentation**: Refer to technical documentation
- **Logs**: Check application and system logs for errors
- **Community**: Join the CannaVille Pro developer community
- **Support**: Contact technical support for critical issues

---

*This guide covers deployment and usage of CannaVille Pro. For additional support or custom deployment scenarios, please consult the technical documentation or contact the development team.*

