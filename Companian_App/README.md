# ⛷️ Biathlon Robo-Coach (Companion App)

This is the AI Companion App for our UTRA Hacks robot. It acts as an "Angry 80s Winter Olympics Coach" that helps debug the robot by roasting the engineering team.

It uses **Google Gemini (or OpenRouter)** for intelligence, **ElevenLabs** for voice synthesis, and simulates **Solana Blockchain** logging for run verification.

## 🚀 Features
* **AI Persona:** An unhinged drill sergeant who gives valid engineering advice wrapped in winter sports insults.
* **Voice Synthesis:** Uses ElevenLabs to scream advice at you in real-time.
* **Blockchain Logging:** Simulates verifying run attempts on the Solana Devnet.
* **Conversation Memory:** The coach remembers previous errors (e.g., if you fixed the sensor mentioned earlier).

## 🛠️ Installation

1.  **Navigate to the app directory:**
    ```bash
    cd Companian_App
    ```

2.  **Install dependencies:**
    ```bash
    pip install -r requirements.txt
    ```

## 🔑 Configuration (IMPORTANT)

This app requires API keys to function. These keys are **ignored by Git** for security. You must create the secrets file manually.

1.  Create a folder named `.streamlit` inside `Companian_App` if it doesn't exist.
2.  Inside that folder, create a file named `secrets.toml`.
3.  Paste the following content into `secrets.toml` and add your keys:

```toml
# OPTION 1: Direct Google Key
GOOGLE_API_KEY = "paste_your_google_gemini_key_here"

# OPTION 2: OpenRouter Key (If using OpenRouter, key starts with sk-or-v1-...)
# GOOGLE_API_KEY = "sk-or-v1-..."

# ElevenLabs Key (Required for Voice)
ELEVENLABS_API_KEY = "paste_your_elevenlabs_key_here"
```
⚠️ WARNING: Never push secrets.toml to GitHub! It is already added to .gitignore.


## ▶️ How to Run
Run the app using Streamlit:

```Bash

python -m streamlit run app.py
```
The app will open automatically in your browser at http://localhost:8501.

## 🐛 Troubleshooting
* "Missing API Key": Make sure you created the .streamlit/secrets.toml file correctly.

* "404 Error (Gemini)": The app automatically attempts to switch models if one is deprecated. If it persists, check your Google Cloud API permissions.

* Audio not playing: Chrome sometimes blocks autoplay. Click "Get Coached" again or interact with the page first.