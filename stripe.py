import stripe
from flask import Blueprint, jsonify, request

stripe_bp = Blueprint("stripe_bp", __name__)

@stripe_bp.route("/config")
def get_publishable_key():
    stripe_config = {"publicKey": stripe.api_key}
    return jsonify(stripe_config)

@stripe_bp.route("/create-checkout-session", methods=["POST"])
def create_checkout_session():
    try:
        checkout_session = stripe.checkout.Session.create(
            line_items=[
                {
                    "price_data": {
                        "currency": "usd",
                        "product_data": {
                            "name": "CannaVille Pro Subscription",
                        },
                        "unit_amount": 2000, # $20.00
                    },
                    "quantity": 1,
                }
            ],
            mode="payment",
            success_url=request.url_root + "success.html",
            cancel_url=request.url_root + "cancel.html",
        )
        return jsonify({"sessionId": checkout_session.id})
    except Exception as e:
        return jsonify(error=str(e)), 403

# Webhook endpoint for Stripe events (e.g., payment success)
@stripe_bp.route("/webhook", methods=["POST"])
def stripe_webhook():
    payload = request.get_data()
    sig_header = request.headers.get("stripe-signature")

    try:
        event = stripe.Webhook.construct_event(
            payload, sig_header, os.environ.get("STRIPE_WEBHOOK_SECRET")
        )
    except ValueError as e:
        # Invalid payload
        return str(e), 400
    except stripe.error.SignatureVerificationError as e:
        # Invalid signature
        return str(e), 400

    # Handle the event
    if event["type"] == "checkout.session.completed":
        session = event["data"]["object"]
        print("Payment successful for session:", session.id)
        # TODO: Fulfill the purchase, e.g., grant access to premium features
    # ... handle other event types

    return jsonify(success=True)


