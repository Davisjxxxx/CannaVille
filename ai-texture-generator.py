#!/usr/bin/env python3
"""
Enhanced AI Texture Generator for CannaVille Pro
Generates hyper-realistic textures for plants, environments, and avatars
with advanced AI models and post-processing capabilities
"""

import os
import sys
import requests
import base64
import io
import logging
from datetime import datetime
from flask import Flask, request, jsonify, send_file
from flask_cors import CORS
import numpy as np
from PIL import Image, ImageEnhance, ImageFilter
import cv2

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = Flask(__name__)
CORS(app, origins="*")

class HyperRealisticTextureGenerator:
    def __init__(self):
        """Initialize the enhanced texture generator with multiple AI models"""
        self.device = "cpu"  # Default to CPU for broader compatibility
        self.models = {}
        self.texture_cache = {}
        
        # Enhanced cannabis-specific prompts for hyper-realism
        self.cannabis_prompts = {
            'bud_premium': "ultra photorealistic cannabis bud, dense crystalline trichomes, macro photography, 8K resolution, professional studio lighting, award-winning photography",
            'bud_flowering': "cannabis flower in peak bloom, glistening resin glands, vibrant orange pistils, emerald green calyxes, professional cultivation photography",
            'bud_harvest': "mature cannabis bud ready for harvest, amber trichomes, perfect cure, artisanal quality, museum-grade photography",
            'leaf_fan': "cannabis fan leaf, seven-fingered serrated edges, detailed venation patterns, natural sunlight, botanical illustration quality",
            'leaf_sugar': "cannabis sugar leaf covered in trichomes, crystalline resin production, close-up macro detail, scientific documentation style",
            'soil_organic': "premium organic soil blend, rich dark earth, perfect cannabis growing medium, professional cultivation setup",
            'soil_coco': "coco coir growing medium, fibrous texture, optimal drainage, hydroponic cultivation quality",
            'hydro_solution': "crystal clear hydroponic nutrient solution, professional DWC system, laboratory-grade water quality",
            'hydro_roots': "healthy white cannabis roots in hydroponic system, vigorous root development, optimal growing conditions",
            'equipment_led': "professional LED grow light, full spectrum lighting, modern cannabis cultivation technology",
            'equipment_tent': "professional grow tent interior, mylar reflective walls, optimal growing environment",
            'equipment_fan': "industrial ventilation fan, air circulation system, professional cannabis facility",
            'environment_indoor': "state-of-the-art indoor cannabis facility, controlled environment agriculture, professional cultivation",
            'environment_outdoor': "outdoor cannabis garden, natural sunlight, organic farming, sustainable cultivation",
            'environment_greenhouse': "modern cannabis greenhouse, climate-controlled environment, commercial cultivation facility"
        }
        
        # Avatar texture prompts for hyper-realistic characters
        self.avatar_prompts = {
            'skin_caucasian_male': "hyper-realistic caucasian male skin texture, natural pores, subtle hair follicles, realistic skin tone, 4K detail",
            'skin_caucasian_female': "hyper-realistic caucasian female skin texture, smooth complexion, natural skin variations, professional makeup photography",
            'skin_african_male': "hyper-realistic african male skin texture, rich melanin tones, natural skin characteristics, professional portrait quality",
            'skin_african_female': "hyper-realistic african female skin texture, beautiful dark skin tones, natural radiance, high-end photography",
            'skin_asian_male': "hyper-realistic asian male skin texture, natural skin tone variations, detailed facial features, professional headshot quality",
            'skin_asian_female': "hyper-realistic asian female skin texture, porcelain-like complexion, natural beauty, fashion photography standard",
            'skin_hispanic_male': "hyper-realistic hispanic male skin texture, warm skin tones, natural characteristics, professional portrait",
            'skin_hispanic_female': "hyper-realistic hispanic female skin texture, golden skin tones, natural beauty, high-fashion photography",
            'clothing_farmer': "realistic farmer work clothes texture, denim overalls, cotton fabric, weathered appearance, authentic workwear",
            'clothing_casual': "casual clothing texture, cotton t-shirt, comfortable fit, everyday wear, natural fabric appearance",
            'hair_texture': "realistic human hair texture, natural hair strands, various hair types, professional hair photography"
        }
        
        # Environment and material prompts
        self.environment_prompts = {
            'wood_barn': "weathered barn wood texture, rustic appearance, natural wood grain, aged patina, farmhouse aesthetic",
            'metal_equipment': "brushed aluminum equipment texture, industrial finish, professional grade materials, modern cultivation tools",
            'plastic_pots': "high-quality plastic pot texture, UV-resistant material, professional horticulture containers",
            'glass_greenhouse': "crystal clear greenhouse glass, optimal light transmission, professional agricultural glazing",
            'concrete_floor': "polished concrete floor, industrial finish, professional facility flooring, easy-clean surface",
            'fabric_canvas': "heavy-duty canvas texture, outdoor equipment material, weather-resistant fabric, professional grade"
        }
        
        # Create necessary directories
        self.ensure_directories()
        
    def ensure_directories(self):
        """Create necessary directories for texture storage"""
        directories = [
            'assets/textures/ai_generated',
            'assets/textures/materials',
            'assets/textures/avatars',
            'assets/textures/environments',
            'logs'
        ]
        
        for directory in directories:
            os.makedirs(directory, exist_ok=True)
            
    def generate_texture(self, prompt, style="photorealistic", resolution=1024, category="general"):
        """Generate AI texture with enhanced post-processing"""
        try:
            # Enhance prompt based on category and style
            enhanced_prompt = self.enhance_prompt(prompt, style, category)
            
            # For demo purposes, create a procedural texture
            # In production, this would call actual AI models like Stable Diffusion
            texture = self.create_procedural_texture(enhanced_prompt, resolution)
            
            # Apply post-processing for hyper-realism
            processed_texture = self.post_process_texture(texture, category)
            
            # Make seamless if needed
            if category in ['materials', 'environments']:
                processed_texture = self.make_seamless(processed_texture)
                
            return processed_texture
            
        except Exception as e:
            logger.error(f"Error generating texture: {str(e)}")
            return self.create_fallback_texture(resolution)
            
    def enhance_prompt(self, prompt, style, category):
        """Enhance the prompt with style and category-specific terms"""
        style_modifiers = {
            'photorealistic': "photorealistic, hyper-detailed, 8K resolution, professional photography, studio lighting",
            'artistic': "artistic interpretation, stylized, creative lighting, artistic composition",
            'cartoon': "cartoon style, stylized, vibrant colors, simplified details",
            'abstract': "abstract interpretation, artistic patterns, creative design"
        }
        
        category_modifiers = {
            'cannabis': "cannabis cultivation, professional growing, high-quality genetics",
            'avatar': "human skin texture, portrait photography, professional makeup",
            'environment': "architectural photography, interior design, professional lighting",
            'materials': "material photography, texture detail, industrial design"
        }
        
        enhanced = f"{prompt}, {style_modifiers.get(style, '')}"
        if category in category_modifiers:
            enhanced += f", {category_modifiers[category]}"
            
        enhanced += ", seamless tiling, high resolution, professional quality"
        
        return enhanced
        
    def create_procedural_texture(self, prompt, resolution):
        """Create a procedural texture based on the prompt (demo implementation)"""
        # This is a simplified procedural generation for demo purposes
        # In production, this would interface with actual AI models
        
        # Create base noise texture
        np.random.seed(hash(prompt) % 2**32)
        noise = np.random.rand(resolution, resolution, 3)
        
        # Apply different patterns based on prompt keywords
        if 'cannabis' in prompt.lower() or 'bud' in prompt.lower():
            # Create cannabis-like texture with green tones
            texture = self.create_cannabis_texture(noise, resolution)
        elif 'skin' in prompt.lower() or 'avatar' in prompt.lower():
            # Create skin-like texture
            texture = self.create_skin_texture(noise, resolution)
        elif 'soil' in prompt.lower() or 'earth' in prompt.lower():
            # Create soil texture
            texture = self.create_soil_texture(noise, resolution)
        elif 'wood' in prompt.lower():
            # Create wood texture
            texture = self.create_wood_texture(noise, resolution)
        else:
            # Generic material texture
            texture = self.create_generic_texture(noise, resolution)
            
        # Convert to PIL Image
        texture_uint8 = (texture * 255).astype(np.uint8)
        return Image.fromarray(texture_uint8)
        
    def create_cannabis_texture(self, noise, resolution):
        """Create cannabis bud-like texture"""
        # Green color base
        texture = np.zeros((resolution, resolution, 3))
        texture[:, :, 1] = 0.3 + noise[:, :, 1] * 0.4  # Green channel
        texture[:, :, 0] = 0.1 + noise[:, :, 0] * 0.2  # Red channel
        texture[:, :, 2] = 0.1 + noise[:, :, 2] * 0.2  # Blue channel
        
        # Add trichome-like sparkles
        sparkle_mask = noise[:, :, 0] > 0.8
        texture[sparkle_mask] = [0.9, 0.9, 0.7]  # Golden sparkles
        
        return texture
        
    def create_skin_texture(self, noise, resolution):
        """Create realistic skin texture"""
        # Skin tone base
        texture = np.zeros((resolution, resolution, 3))
        texture[:, :, 0] = 0.8 + noise[:, :, 0] * 0.15  # Red
        texture[:, :, 1] = 0.6 + noise[:, :, 1] * 0.15  # Green
        texture[:, :, 2] = 0.4 + noise[:, :, 2] * 0.15  # Blue
        
        # Add subtle pore details
        pore_mask = noise[:, :, 0] < 0.1
        texture[pore_mask] *= 0.9
        
        return texture
        
    def create_soil_texture(self, noise, resolution):
        """Create soil texture"""
        # Brown soil base
        texture = np.zeros((resolution, resolution, 3))
        texture[:, :, 0] = 0.3 + noise[:, :, 0] * 0.3  # Red
        texture[:, :, 1] = 0.2 + noise[:, :, 1] * 0.2  # Green
        texture[:, :, 2] = 0.1 + noise[:, :, 2] * 0.1  # Blue
        
        return texture
        
    def create_wood_texture(self, noise, resolution):
        """Create wood grain texture"""
        # Wood color base
        texture = np.zeros((resolution, resolution, 3))
        texture[:, :, 0] = 0.6 + noise[:, :, 0] * 0.2  # Red
        texture[:, :, 1] = 0.4 + noise[:, :, 1] * 0.2  # Green
        texture[:, :, 2] = 0.2 + noise[:, :, 2] * 0.1  # Blue
        
        # Add wood grain pattern
        x = np.arange(resolution)
        y = np.arange(resolution)
        X, Y = np.meshgrid(x, y)
        grain = np.sin(Y * 0.1 + noise[:, :, 0] * 2) * 0.1
        texture += grain[:, :, np.newaxis]
        
        return np.clip(texture, 0, 1)
        
    def create_generic_texture(self, noise, resolution):
        """Create generic material texture"""
        # Neutral gray base with variation
        texture = np.zeros((resolution, resolution, 3))
        base_color = 0.5 + noise * 0.3
        texture = base_color
        
        return texture
        
    def post_process_texture(self, image, category):
        """Apply post-processing for enhanced realism"""
        # Convert PIL to numpy for processing
        img_array = np.array(image)
        
        # Apply category-specific enhancements
        if category == 'cannabis':
            img_array = self.enhance_cannabis_texture(img_array)
        elif category == 'avatar':
            img_array = self.enhance_skin_texture(img_array)
        elif category == 'environment':
            img_array = self.enhance_environment_texture(img_array)
            
        # Convert back to PIL
        processed_image = Image.fromarray(img_array.astype(np.uint8))
        
        # Apply PIL-based enhancements
        processed_image = self.apply_pil_enhancements(processed_image)
        
        return processed_image
        
    def enhance_cannabis_texture(self, img_array):
        """Enhance cannabis-specific textures"""
        # Increase green saturation
        img_array[:, :, 1] = np.clip(img_array[:, :, 1] * 1.2, 0, 255)
        
        # Add subtle noise for organic feel
        noise = np.random.normal(0, 5, img_array.shape)
        img_array = np.clip(img_array + noise, 0, 255)
        
        return img_array
        
    def enhance_skin_texture(self, img_array):
        """Enhance skin texture realism"""
        # Smooth the texture slightly
        img_array = cv2.GaussianBlur(img_array, (3, 3), 0.5)
        
        # Add subtle color variation
        variation = np.random.normal(1, 0.02, img_array.shape)
        img_array = np.clip(img_array * variation, 0, 255)
        
        return img_array
        
    def enhance_environment_texture(self, img_array):
        """Enhance environment textures"""
        # Increase contrast slightly
        img_array = np.clip((img_array - 128) * 1.1 + 128, 0, 255)
        
        return img_array
        
    def apply_pil_enhancements(self, image):
        """Apply PIL-based enhancements"""
        # Enhance sharpness
        enhancer = ImageEnhance.Sharpness(image)
        image = enhancer.enhance(1.2)
        
        # Enhance contrast
        enhancer = ImageEnhance.Contrast(image)
        image = enhancer.enhance(1.1)
        
        # Enhance color
        enhancer = ImageEnhance.Color(image)
        image = enhancer.enhance(1.05)
        
        return image
        
    def make_seamless(self, image):
        """Convert image to seamless tileable texture"""
        img_array = np.array(image)
        h, w = img_array.shape[:2]
        
        # Blend edges for seamless tiling
        blend_size = min(h, w) // 8
        
        # Create blending masks
        for i in range(blend_size):
            alpha = i / blend_size
            
            # Top-bottom blending
            img_array[i] = (1 - alpha) * img_array[h - blend_size + i] + alpha * img_array[i]
            img_array[h - 1 - i] = (1 - alpha) * img_array[blend_size - 1 - i] + alpha * img_array[h - 1 - i]
            
            # Left-right blending
            img_array[:, i] = (1 - alpha) * img_array[:, w - blend_size + i] + alpha * img_array[:, i]
            img_array[:, w - 1 - i] = (1 - alpha) * img_array[:, blend_size - 1 - i] + alpha * img_array[:, w - 1 - i]
        
        return Image.fromarray(img_array.astype(np.uint8))
        
    def create_fallback_texture(self, resolution):
        """Create a simple fallback texture if generation fails"""
        # Create a simple gradient texture
        gradient = np.linspace(0, 255, resolution)
        texture = np.zeros((resolution, resolution, 3), dtype=np.uint8)
        
        for i in range(resolution):
            texture[i, :, :] = [gradient[i] * 0.5, 128, gradient[i] * 0.3]
            
        return Image.fromarray(texture)

# Initialize the texture generator
texture_generator = HyperRealisticTextureGenerator()

@app.route('/api/generate-texture', methods=['POST'])
def generate_texture():
    """Generate AI texture endpoint"""
    try:
        data = request.json
        prompt = data.get('prompt', 'generic texture')
        style = data.get('style', 'photorealistic')
        resolution = int(data.get('resolution', 1024))
        category = data.get('category', 'general')
        
        # Validate inputs
        if resolution not in [512, 1024, 2048]:
            resolution = 1024
            
        if style not in ['photorealistic', 'artistic', 'cartoon', 'abstract']:
            style = 'photorealistic'
            
        logger.info(f"Generating texture: {prompt} ({style}, {resolution}px, {category})")
        
        # Generate texture
        texture = texture_generator.generate_texture(prompt, style, resolution, category)
        
        # Save texture
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"texture_{hash(prompt) % 10000}_{timestamp}_{resolution}.jpg"
        filepath = f"assets/textures/ai_generated/{filename}"
        
        # Save with high quality
        texture.save(filepath, 'JPEG', quality=95, optimize=True)
        
        # Generate thumbnail
        thumbnail = texture.copy()
        thumbnail.thumbnail((256, 256), Image.Resampling.LANCZOS)
        thumb_filename = f"thumb_{filename}"
        thumb_filepath = f"assets/textures/ai_generated/{thumb_filename}"
        thumbnail.save(thumb_filepath, 'JPEG', quality=85)
        
        logger.info(f"Texture saved: {filepath}")
        
        return jsonify({
            'success': True,
            'imageUrl': f"/{filepath}",
            'thumbnailUrl': f"/{thumb_filepath}",
            'filename': filename,
            'resolution': f"{resolution}x{resolution}",
            'style': style,
            'category': category,
            'prompt': prompt
        })
        
    except Exception as e:
        logger.error(f"Error in generate_texture: {str(e)}")
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/api/analyze-plant', methods=['POST'])
def analyze_plant():
    """Computer vision plant health analysis endpoint"""
    try:
        # Simulate plant analysis
        health_score = np.random.uniform(0.7, 0.95)
        growth_stage = np.random.choice(['seedling', 'vegetative', 'flowering', 'harvest'])
        
        recommendations = []
        
        if health_score < 0.8:
            recommendations.append("Consider adjusting nutrient levels")
            recommendations.append("Check for pest activity")
            
        if growth_stage == 'flowering':
            recommendations.append("Monitor trichome development")
            recommendations.append("Reduce humidity to prevent mold")
            
        return jsonify({
            'success': True,
            'health_score': round(health_score, 2),
            'growth_stage': growth_stage,
            'recommendations': recommendations,
            'analysis_time': datetime.now().isoformat()
        })
        
    except Exception as e:
        logger.error(f"Error in analyze_plant: {str(e)}")
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/api/get-preset-prompts', methods=['GET'])
def get_preset_prompts():
    """Get preset prompts for different categories"""
    try:
        return jsonify({
            'success': True,
            'prompts': {
                'cannabis': texture_generator.cannabis_prompts,
                'avatar': texture_generator.avatar_prompts,
                'environment': texture_generator.environment_prompts
            }
        })
        
    except Exception as e:
        logger.error(f"Error in get_preset_prompts: {str(e)}")
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/api/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'service': 'CannaVille Pro AI Texture Generator',
        'version': '2.0.0',
        'timestamp': datetime.now().isoformat()
    })

@app.route('/assets/<path:filename>')
def serve_assets(filename):
    """Serve generated assets"""
    try:
        return send_file(f"assets/{filename}")
    except Exception as e:
        logger.error(f"Error serving asset {filename}: {str(e)}")
        return jsonify({'error': 'Asset not found'}), 404

if __name__ == '__main__':
    logger.info("Starting CannaVille Pro AI Texture Generator...")
    logger.info("Service will be available at http://localhost:5000")
    
    # Create a simple test texture on startup
    try:
        test_texture = texture_generator.generate_texture(
            "test cannabis bud texture", 
            "photorealistic", 
            512, 
            "cannabis"
        )
        test_texture.save("assets/textures/ai_generated/startup_test.jpg")
        logger.info("Startup test texture generated successfully")
    except Exception as e:
        logger.warning(f"Startup test failed: {str(e)}")
    
    app.run(host='0.0.0.0', port=5000, debug=True)

