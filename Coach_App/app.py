import streamlit as st
import requests
import base64
import json
import time
import os

# --- CONFIGURATION ---
st.set_page_config(page_title="Biathlon Coach", page_icon="❄️", layout="centered")

# --- DARK ICE THEME CSS ---
st.markdown("""
<style>
    /* 1. MAIN BACKGROUND */
    .stApp {
        background: radial-gradient(circle at center, #1e3a8a 0%, #020617 100%);
        background-attachment: fixed;
    }

    /* 2. INPUT FIELD */
    .stTextInput input {
        background-color: rgba(30, 41, 59, 0.8) !important;
        color: #e0f2fe !important;
        border: 2px solid #38bdf8 !important;
        border-radius: 8px;
    }
    .stTextInput label {
        color: #38bdf8 !important;
        font-weight: 800 !important;
        font-size: 1.5rem !important;
        text-transform: uppercase;
        text-shadow: 0 0 5px rgba(56, 189, 248, 0.5);
    }

    /* 3. SUBMIT BUTTON */
    .stButton button {
        width: 100%;
        border-radius: 8px;
        font-weight: 900;
        text-transform: uppercase;
    }
    div[data-testid="stForm"] button {
        background: linear-gradient(180deg, #FFD700 0%, #B8860B 100%);
        color: black !important;
        border: 2px solid #DAA520;
        box-shadow: 0px 0px 15px rgba(255, 215, 0, 0.4);
    }
    div[data-testid="stForm"] button:hover {
        transform: scale(1.02);
        box-shadow: 0px 0px 25px rgba(255, 215, 0, 0.7);
    }

    /* 4. CHAT LOG */
    .chat-container {
        background: rgba(0, 0, 0, 0.3);
        border: 1px solid rgba(56, 189, 248, 0.2);
        border-radius: 15px;
        padding: 20px;
        max-height: 400px;
        overflow-y: auto;
        margin-bottom: 20px;
        box-shadow: inset 0 0 20px rgba(0,0,0,0.5);
        display: flex;
        flex-direction: column;
        gap: 15px;
    }
    .coach-row { display: flex; align-items: flex-start; gap: 10px; }
    .coach-avatar { font-size: 24px; filter: drop-shadow(0 0 5px #dc2626); }
    .coach-bubble {
        background: linear-gradient(135deg, rgba(220, 38, 38, 0.2), rgba(69, 10, 10, 0.4));
        border: 1px solid #dc2626;
        color: #fee2e2;
        padding: 12px 16px;
        border-radius: 0px 15px 15px 15px;
        max-width: 85%;
        backdrop-filter: blur(4px);
        box-shadow: 0 4px 6px rgba(0,0,0,0.2);
        font-family: monospace;
    }
    .user-row { display: flex; flex-direction: row-reverse; align-items: flex-start; gap: 10px; }
    .user-avatar { font-size: 24px; filter: drop-shadow(0 0 5px #0ea5e9); }
    .user-bubble {
        background: linear-gradient(135deg, rgba(14, 165, 233, 0.2), rgba(8, 47, 73, 0.4));
        border: 1px solid #0ea5e9;
        color: #e0f2fe;
        padding: 12px 16px;
        border-radius: 15px 0px 15px 15px;
        max-width: 85%;
        backdrop-filter: blur(4px);
        box-shadow: 0 4px 6px rgba(0,0,0,0.2);
        text-align: right;
    }
    
    /* 5. HEADER */
    .header-container {
        text-align: center;
        padding: 25px;
        background: linear-gradient(180deg, rgba(15, 23, 42, 0) 0%, rgba(56, 189, 248, 0.1) 100%);
        border-bottom: 1px solid #38bdf8;
        margin-bottom: 25px;
        border-radius: 0 0 20px 20px;
    }
    
    /* 6. TYPING CONTAINER (Fixed Box) */
    .typing-box {
        background: rgba(0, 0, 0, 0.8); 
        border: 2px solid #22d3ee; 
        padding: 15px; 
        border-radius: 10px; 
        min-height: 80px; 
        box-shadow: 0px 0px 15px rgba(34, 211, 238, 0.3); 
        backdrop-filter: blur(5px);
        color: #22d3ee;
        font-family: 'Courier New', monospace;
        font-size: 1.2rem;
        font-weight: bold;
        text-transform: uppercase;
        line-height: 1.4;
        text-shadow: 0 0 5px #22d3ee;
        margin-bottom: 20px;
        white-space: pre-wrap; /* Keeps text formatting */
    }
</style>
""", unsafe_allow_html=True)

# Check for keys
if "GOOGLE_API_KEY" not in st.secrets:
    st.error("Missing API Key in .streamlit/secrets.toml")
    st.stop()

USE_OPENROUTER = st.secrets["GOOGLE_API_KEY"].startswith("sk-or-v1-")
VOICE_ID = "JBFqnCBsd6RMkjVDRZzb" 

# --- SMART COACH PROMPT ---
SYSTEM_PROMPT = """
You are "COACH AVALANCHE," an unhinged, screaming Winter Olympics Robotics Coach.
You are stuck in the 1980s. You hate weakness. You love gold medals.
The user is a rookie engineer competing in the "Biathlon Bot" Hackathon.

YOUR KNOWLEDGE BASE (BASED ON THE HACKER KIT):
- CONTROLLER: Arduino Uno (USB-C). 2KB SRAM. Don't waste memory!
- POWER: 4x 9V Batteries. These drain fast. IF IT REBOOTS, IT'S THE BATTERY.
- SENSORS: 
  * 1x Ultrasonic (HC-SR04): For dodging obstacles.
  * 1x Color Sensor: PRIMARY EYE for the Green/Red Lanes and Target Zones.
  * 2x IR Sensors: Backup detection or short-range vision.
- ACTUATORS: 
  * 2x DC Motors (with Wheels) & 1x Motor Driver (H-Bridge).
  * 2x Servos: For the "Arm and Claw" or "Shooting Mechanism".
- STRUCTURE: Laser Cut Base + Medium Breadboard.

HOW TO RESPOND:
1. IF VAGUE: EXPLODE. Ask about wiring, loose screws, or dead 9V batteries.
2. IF SPECIFIC: DIAGNOSE (e.g. "Servos twitching? That's brownout!"), ROAST, then SOLVE.
3. SPECIFIC ADVICE: 
   - If they miss the line, blame the COLOR SENSOR THRESHOLD or LIGHTING.
   - If the arm/claw fails, blame the SERVO POWER (current draw).

Keep it under 60 words. ALL CAPS.
"""

API_KEY = st.secrets["GOOGLE_API_KEY"]

# API SETUP
if USE_OPENROUTER:
    OPENROUTER_API_URL = "https://openrouter.ai/api/v1/chat/completions"
    MODEL_NAME = "google/gemini-2.0-flash-001"
else:
    import google.generativeai as genai
    genai.configure(api_key=API_KEY)
    @st.cache_data
    def get_available_model():
        try:
            available_models = list(genai.list_models())
            supported_models = [m for m in available_models if 'generateContent' in m.supported_generation_methods]
            if not supported_models: return None, "No models found"
            return supported_models[0].name.split('/')[-1], None
        except Exception as e: return None, str(e)
    model_name_used, error = get_available_model()
    if not model_name_used: st.stop()
    try: model = genai.GenerativeModel(model_name_used, system_instruction=SYSTEM_PROMPT)
    except: model = genai.GenerativeModel(model_name_used)

if 'conversation_messages' not in st.session_state: st.session_state.conversation_messages = []
if 'chat_session' not in st.session_state: st.session_state.chat_session = None

# --- FUNCTIONS ---
def get_coach_rant(user_input, chat_session=None, is_first_message=False):
    formatted_input = f"Rookie Status: {user_input}"
    try:
        if USE_OPENROUTER:
            messages = [{"role": "system", "content": SYSTEM_PROMPT}]
            if 'conversation_messages' in st.session_state:
                for msg in st.session_state.conversation_messages:
                    role = "assistant" if msg['role'] == 'coach' else "user"
                    messages.append({"role": role, "content": msg['content']})
            messages.append({"role": "user", "content": formatted_input})
            headers = {"Authorization": f"Bearer {API_KEY}", "Content-Type": "application/json", "HTTP-Referer": "https://streamlit.io/", "X-Title": "Biathlon Coach"}
            payload = {"model": MODEL_NAME, "messages": messages}
            response = requests.post(OPENROUTER_API_URL, headers=headers, json=payload, timeout=10)
            if response.status_code == 200: return response.json()["choices"][0]["message"]["content"]
            else: return "I'M TOO ANGRY TO CONNECT! (API Error)"
        else:
            if chat_session: response = chat_session.send_message(formatted_input)
            else: response = model.generate_content(formatted_input)
            return response.text
    except Exception as e: return f"SYSTEM FAILURE! {e}"

def speak_text(text):
    if "ELEVENLABS_API_KEY" not in st.secrets:
        st.warning("No ElevenLabs Key found. Audio disabled.")
        return None
        
    url = f"https://api.elevenlabs.io/v1/text-to-speech/{VOICE_ID}"
    headers = {
        "xi-api-key": st.secrets["ELEVENLABS_API_KEY"], 
        "Content-Type": "application/json"
    }
    data = {
        "text": text,
        "model_id": "eleven_flash_v2_5", 
        "voice_settings": {"stability": 0.5, "similarity_boost": 0.8}
    }
    try:
        response = requests.post(url, json=data, headers=headers, timeout=5)
        if response.status_code == 200:
            return response.content
        else:
            st.error(f"Voice Error: {response.text}")
            return None
    except Exception as e:
        st.error(f"Connection Error: {e}")
        return None

# --- APP UI START ---

# 1. HEADER
col1, col2, col3 = st.columns([1, 2, 1])
with col2:
    if os.path.exists("logo.png"): st.image("logo.png", width=300)
    else: st.image("https://upload.wikimedia.org/wikipedia/commons/thumb/5/5c/Olympic_rings_without_rims.svg/1200px-Olympic_rings_without_rims.svg.png", width=300)

st.markdown("""
<div class="header-container">
    <h1 style="color: #ffffff; margin: 0; font-family: 'Arial Black', sans-serif; font-style: italic; text-transform: uppercase; font-size: 2.5rem; text-shadow: 0px 0px 15px #38bdf8;">
        OLYMPIC ROBO-COACH
    </h1>
    <h3 style="color: #FFD700; margin-top: 5px; font-style: italic; text-shadow: 0px 0px 5px rgba(255, 215, 0, 0.5);">
        "GOLD MEDALS AREN'T WON BY WEAK ROBOTS!"
    </h3>
</div>
""", unsafe_allow_html=True)

# 2. CHAT HISTORY (Shows only PREVIOUS messages)
if st.session_state.conversation_messages:
    chat_html = '<div class="chat-container">'
    for msg in st.session_state.conversation_messages:
        if msg['role'] == 'user':
            chat_html += f"""
<div class="user-row">
    <div class="user-avatar">🎿</div>
    <div class="user-bubble">{msg['content']}</div>
</div>
"""
        else:
            chat_html += f"""
<div class="coach-row">
    <div class="coach-avatar">📢</div>
    <div class="coach-bubble"><b>COACH:</b><br>{msg['content']}</div>
</div>
"""
    chat_html += '</div>'
    st.markdown(chat_html, unsafe_allow_html=True)

# 3. RESPONSE PLACEHOLDER (Active Rant appears here)
response_placeholder = st.empty()

# 4. INPUT FORM
with st.container():
    with st.form(key='chat_form', clear_on_submit=True):
        user_input = st.text_input("VENT YOUR PROBLEMS TO COACH:", placeholder="e.g., The robot missed the green line!")
        col1, col2 = st.columns([3, 1])
        with col1:
            submit_btn = st.form_submit_button("GET COACHED 🥇")
            
    if st.button("🔄 RESET", type="secondary"):
        st.session_state.conversation_messages = []
        st.session_state.chat_session = None
        st.rerun()

# --- LOGIC ---
if submit_btn and user_input:
    if st.session_state.chat_session is None and not USE_OPENROUTER:
        try: st.session_state.chat_session = model.start_chat()
        except: pass

    # 1. Update History (Backend)
    st.session_state.conversation_messages.append({'role': 'user', 'content': user_input})
    
    # 2. Get Response
    with response_placeholder.container():
        # Show User Input immediately in the active area
        st.markdown(f'<div class="user-row"><div class="user-avatar">🎿</div><div class="user-bubble">{user_input}</div></div>', unsafe_allow_html=True)
        
        with st.spinner("❄️ Coach is sharpening his skates..."):
            rant_text = get_coach_rant(user_input, st.session_state.chat_session)
            
    # 3. Get Audio
    with response_placeholder.container():
        # Keep user input visible
        st.markdown(f'<div class="user-row"><div class="user-avatar">🎿</div><div class="user-bubble">{user_input}</div></div>', unsafe_allow_html=True)
        
        with st.spinner("🔊 Generating Scream..."):
            audio_data = speak_text(rant_text)

    # 4. Show Result (No Rerun)
    with response_placeholder.container():
        # A. Show User Input (Again, to keep it stable)
        st.markdown(f'<div class="user-row"><div class="user-avatar">🎿</div><div class="user-bubble">{user_input}</div></div>', unsafe_allow_html=True)
        
        st.markdown("### 🗣️ COACH IS SCREAMING:")
        
        # B. Play Audio
        if audio_data:
            audio_base64 = base64.b64encode(audio_data).decode('utf-8')
            audio_html = f"""
                <audio autoplay>
                    <source src="data:audio/mp3;base64,{audio_base64}" type="audio/mp3">
                </audio>
            """
            st.markdown(audio_html, unsafe_allow_html=True)
        
        # C. Python Typing Effect (INSIDE BOX + SLOWER)
        text_box = st.empty()
        full_text = ""
        
        # Speed: 0.35s is ideal for "Angry Coach" pace
        for word in rant_text.split():
            full_text += word + " "
            # This HTML string updates the same box over and over
            text_box.markdown(f'<div class="typing-box">{full_text}</div>', unsafe_allow_html=True)
            time.sleep(0.35) 
            
    # 5. Save to History (So it appears in the top log NEXT time)
    st.session_state.conversation_messages.append({'role': 'coach', 'content': rant_text})
    
    # NO ST.RERUN() - This keeps the audio player alive!