from flask import Blueprint, jsonify, request
from src.models.game_data import GameState
from src.models.user import User, db

game_bp = Blueprint("game_bp", __name__)

@game_bp.route("/save_game", methods=["POST"])
def save_game():
    data = request.json
    user_id = data.get("user_id")
    game_state_data = data.get("game_state")

    if not user_id or not game_state_data:
        return jsonify({"error": "Missing user_id or game_state"}), 400

    game_state = GameState.query.filter_by(user_id=user_id).first()
    if game_state:
        game_state.money = game_state_data.get("money", game_state.money)
        game_state.energy = game_state_data.get("energy", game_state.energy)
        game_state.level = game_state_data.get("level", game_state.level)
        game_state.day = game_state_data.get("day", game_state.day)
        game_state.environment = game_state_data.get("environment", game_state.environment)
        game_state.growth_stage = game_state_data.get("growth_stage", game_state.growth_stage)
        game_state.inventory = game_state_data.get("inventory", game_state.inventory)
        game_state.environment_settings = game_state_data.get("environment_settings", game_state.environment_settings)
    else:
        game_state = GameState(
            user_id=user_id,
            money=game_state_data.get("money"),
            energy=game_state_data.get("energy"),
            level=game_state_data.get("level"),
            day=game_state_data.get("day"),
            environment=game_state_data.get("environment"),
            growth_stage=game_state_data.get("growth_stage"),
            inventory=game_state_data.get("inventory"),
            environment_settings=game_state_data.get("environment_settings")
        )
        db.session.add(game_state)

    db.session.commit()
    return jsonify({"message": "Game state saved successfully"}), 200

@game_bp.route("/load_game/<int:user_id>", methods=["GET"])
def load_game(user_id):
    game_state = GameState.query.filter_by(user_id=user_id).first()
    if game_state:
        return jsonify({
            "money": game_state.money,
            "energy": game_state.energy,
            "level": game_state.level,
            "day": game_state.day,
            "environment": game_state.environment,
            "growth_stage": game_state.growth_stage,
            "inventory": game_state.inventory,
            "environment_settings": game_state.environment_settings
        }), 200
    else:
        return jsonify({"error": "Game state not found for this user"}), 404


