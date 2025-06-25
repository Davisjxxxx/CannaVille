from src.models.user import db
from flask_login import UserMixin

class User(UserMixin, db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)
    email = db.Column(db.String(120), unique=True, nullable=False)
    password = db.Column(db.String(120), nullable=False)

    def __repr__(self):
        return 

class GameState(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey("user.id"), nullable=False)
    money = db.Column(db.Float, default=1000.0)
    energy = db.Column(db.Integer, default=100)
    level = db.Column(db.Integer, default=1)
    day = db.Column(db.Integer, default=1)
    environment = db.Column(db.String(50), default="indoor")
    growth_stage = db.Column(db.String(50), default="vegetative")
    inventory = db.Column(db.JSON) # Store inventory as JSON
    environment_settings = db.Column(db.JSON) # Store environment settings as JSON

    def __repr__(self):
        return 


