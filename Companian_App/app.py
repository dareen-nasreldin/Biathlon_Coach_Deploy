import streamlit as st
import requests
import base64
import json
import uuid
import os

# --- CONFIGURATION ---
st.set_page_config(page_title="Biathlon Coach", page_icon="⛷️", layout="centered")

# --- DARK MODE OLYMPIC CSS ---
st.markdown("""
<style>
    /* 1. MAIN BACKGROUND: Dark Winter Night */
    .stApp {
        background-color: #0e1117;
        background-image: radial-gradient(#1c222e 10%, transparent 10%), radial-gradient(#1c222e 10%, transparent 10%);
        background-size: 30px 30px;
        background-position: 0 0, 15px 15px;
    }

    /* 2. INPUT FIELD (Dark Mode) */
    .stTextInput input {
        background-color: #262730 !important;
        color: #ffffff !important;
        border: 2px solid #4da6ff !important; /* Neon Blue Border */
        border-radius: 8px;
    }
    .stTextInput label {
        color: #4da6ff !important;
        font-weight: 800 !important;
        font-size: 1rem !important;
        text-transform: uppercase;
    }

    /* 3. PRIMARY BUTTON (GOLD MEDAL) */
    div.stButton > button[kind="primary"] {
        background: linear-gradient(180deg, #FFD700 0%, #B8860B 100%);
        color: black !important;
        border: 2px solid #DAA520;
        font-weight: 900;
        text-transform: uppercase;
        font-size: 1.2rem;
        border-radius: 8px;
        width: 100%;
        box-shadow: 0px 0px 10px rgba(255, 215, 0, 0.5); /* Gold Glow */
        transition: all 0.2s;
    }
    div.stButton > button[kind="primary"]:hover {
        transform: scale(1.02);
        box-shadow: 0px 0px 20px rgba(255, 215, 0, 0.8);
    }

    /* 4. SECONDARY BUTTON (RESET - Red Glow) */
    div.stButton > button[kind="secondary"] {
        background-color: transparent;
        border: 2px solid #FF4B4B; 
        color: #FF4B4B !important;
        font-weight: bold;
        border-radius: 8px;
        width: 100%;
    }
    div.stButton > button[kind="secondary"]:hover {
        border-color: #FF0000;
        color: #FF0000 !important;
        box-shadow: 0px 0px 10px rgba(255, 0, 0, 0.5);
    }

    /* 5. CHAT BUBBLES (Dark Mode Style) */
    .user-msg {
        background-color: #1e3a8a; /* Dark Blue */
        color: #e0f2fe;
        padding: 15px;
        border-radius: 15px;
        margin-bottom: 10px;
        border-left: 4px solid #3b82f6;
        box-shadow: 2px 2px 10px rgba(0,0,0,0.3);
    }
    .coach-msg {
        background-color: #450a0a; /* Dark Red */
        color: #ffe4e6;
        padding: 15px;
        border-radius: 15px;
        margin-bottom: 10px;
        border-left: 4px solid #ef4444;
        box-shadow: 2px 2px 10px rgba(0,0,0,0.3);
    }
    
    /* 6. HEADER CONTAINER */
    .header-container {
        text-align: center;
        padding: 20px;
        background: linear-gradient(180deg, rgba(0,0,0,0) 0%, rgba(0,47,108,0.3) 100%);
        border-bottom: 2px solid #4da6ff;
        margin-bottom: 20px;
    }
</style>
""", unsafe_allow_html=True)

# Check for keys
if "GOOGLE_API_KEY" not in st.secrets:
    st.error("Missing API Key in .streamlit/secrets.toml")
    st.stop()

# Detect if using OpenRouter
USE_OPENROUTER = st.secrets["GOOGLE_API_KEY"].startswith("sk-or-v1-")
VOICE_ID = "JBFqnCBsd6RMkjVDRZzb" 

# SYSTEM PROMPT
SYSTEM_PROMPT = """
You are a PSYCHOTIC, SCREAMING Winter Olympics Coach who hates failure.
The user is a pathetic rookie engineer whose robot is garbage. 
You have FULL MEMORY of the conversation.

MODE 1 - CLARIFICATION (If input is vague):
- SCREAM questions about sensors, motors, or code.
- "I CAN'T FIX WHAT I CAN'T SEE!"

MODE 2 - SOLUTION (If input has details):
- ROAST them with winter sports metaphors.
- Give SPECIFIC engineering advice.
- THREATEN to kick them off the team.

Always maintain the angry, demanding coach personality in ALL CAPS.
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
        except Exception as e:
            return None, str(e)
    
    model_name_used, error = get_available_model()
    if not model_name_used: st.stop()
    
    try:
        model = genai.GenerativeModel(model_name_used, system_instruction=SYSTEM_PROMPT)
    except:
        model = genai.GenerativeModel(model_name_used)

    if 'model_name_used' not in st.session_state:
        st.session_state.model_name_used = model_name_used

# --- FUNCTIONS ---
def is_vague_input(text):
    text_lower = text.lower()
    word_count = len(text.split())
    specific_terms = ["sensor", "motor", "servo", "code", "pin", "voltage", "speed", "turn", "stop", "move", "track", "line", "color", "ultrasonic"]
    has_specific = any(t in text_lower for t in specific_terms)
    if word_count < 4 and not has_specific: return True
    return False

def get_coach_rant(user_input, chat_session=None, is_first_message=False):
    if is_first_message or is_vague_input(user_input):
        formatted_input = f"Rookie Status: {user_input}\nCoach Response (MODE 1 - Ask Details):"
    else:
        formatted_input = f"Rookie Status: {user_input}\nCoach Response (MODE 2 - Solution):"

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
            if response.status_code == 200:
                return response.json()["choices"][0]["message"]["content"]
            else:
                return "I'M TOO ANGRY TO CONNECT! (API Error)"
        else:
            if chat_session:
                response = chat_session.send_message(formatted_input)
            else:
                response = model.generate_content(formatted_input)
            return response.text
    except Exception as e:
        return f"SYSTEM FAILURE! {e}"

def speak_text(text):
    if "ELEVENLABS_API_KEY" not in st.secrets: return None
    url = f"https://api.elevenlabs.io/v1/text-to-speech/{VOICE_ID}"
    headers = {"xi-api-key": st.secrets["ELEVENLABS_API_KEY"], "Content-Type": "application/json"}
    data = {"text": text, "model_id": "eleven_flash_v2_5", "voice_settings": {"stability": 0.5, "similarity_boost": 0.8}}
    try:
        response = requests.post(url, json=data, headers=headers, timeout=5)
        return response.content if response.status_code == 200 else None
    except:
        return None

# --- APP UI ---
if 'conversation_messages' not in st.session_state:
    st.session_state.conversation_messages = []
if 'chat_session' not in st.session_state:
    st.session_state.chat_session = None

# --- LOGO & HEADER ---
col1, col2, col3 = st.columns([1, 2, 1])
with col2:
    # Look for 'logo.png' locally, otherwise show a web placeholder
    if os.path.exists("logo.png"):
        st.image("logo.png", use_container_width=True)
    else:
        # Fallback to generic Olympic rings if they haven't added the file yet
        st.image("https://upload.wikimedia.org/wikipedia/commons/thumb/5/5c/Olympic_rings_without_rims.svg/1200px-Olympic_rings_without_rims.svg.png", use_container_width=True)

st.markdown("""
<div class="header-container">
    <h1 style="color: #ffffff; margin: 0; font-family: 'Arial Black', sans-serif; font-style: italic; text-transform: uppercase; font-size: 2.5rem; text-shadow: 0px 0px 10px #4da6ff;">
        OLYMPIC ROBO-COACH
    </h1>
    <h3 style="color: #FFD700; margin-top: 5px; font-style: italic;">
        "GOLD MEDALS AREN'T WON BY WEAK ROBOTS!"
    </h3>
</div>
""", unsafe_allow_html=True)

# HISTORY
if st.session_state.conversation_messages:
    with st.expander("📜 TRAINING LOG (History)", expanded=True):
        for msg in st.session_state.conversation_messages:
            if msg['role'] == 'user':
                st.markdown(f'<div class="user-msg"><b>🎿 ROOKIE:</b><br>{msg["content"]}</div>', unsafe_allow_html=True)
            else:
                st.markdown(f'<div class="coach-msg"><b>📢 COACH:</b><br>{msg["content"]}</div>', unsafe_allow_html=True)

with st.container():
    # INPUT SECTION
    user_input = st.text_input("📢 REPORT SLOPE CONDITIONS:", placeholder="e.g., The robot missed the green line!", key="user_input")

    # BUTTONS
    col1, col2 = st.columns([3, 1])
    with col1:
        rant_btn = st.button("GET COACHED 🥇", type="primary")
    with col2:
        if st.button("🔄 RESET", type="secondary", help="Clear Conversation"):
            st.session_state.conversation_messages = []
            st.session_state.chat_session = None
            st.rerun()

if rant_btn and user_input:
    if st.session_state.chat_session is None and not USE_OPENROUTER:
        try:
            st.session_state.chat_session = model.start_chat()
        except:
            pass

    st.session_state.conversation_messages.append({'role': 'user', 'content': user_input})
    
    with st.spinner("😤 Coach is waxing his skis..."):
        is_first = len(st.session_state.conversation_messages) == 1
        rant_text = get_coach_rant(user_input, st.session_state.chat_session, is_first)
    
    st.session_state.conversation_messages.append({'role': 'coach', 'content': rant_text})
    
    # 2. Get Audio
    with st.spinner("🔊 Generating Scream..."):
        audio_data = speak_text(rant_text)

    # 3. Display Result
    st.markdown("### 🗣️ COACH IS SCREAMING:")
    
    if audio_data:
        # TYPING EFFECT + AUDIO
        words = rant_text.split()
        words_json = json.dumps(words)
        audio_base64 = base64.b64encode(audio_data).decode('utf-8')
        unique_id = str(uuid.uuid4())[:8]
        
        typing_html = f"""
        <html>
        <style>
            @import url('https://fonts.googleapis.com/css2?family=VT323&display=swap');
            body {{ background: transparent; margin: 0; overflow: hidden; }}
            #typingContainer {{ 
                background: #000000; 
                border: 2px solid #EE334E; 
                padding: 15px; 
                border-radius: 10px; 
                min-height: 100px;
                box-shadow: 0px 0px 15px rgba(238, 51, 78, 0.4);
            }}
            #text {{ 
                color: #39ff14; /* NEON GREEN TEXT */
                font-family: 'VT323', monospace; 
                font-size: 22px; 
                text-transform: uppercase; 
                line-height: 1.4; 
                text-shadow: 0 0 5px #39ff14;
            }}
        </style>
        <body>
            <div id="typingContainer"><div id="text"></div></div>
            <audio id="audio" autoplay src="data:audio/mp3;base64,{audio_base64}"></audio>
            <script>
                var words = {words_json};
                var el = document.getElementById('text');
                var audio = document.getElementById('audio');
                
                setTimeout(function() {{
                    if(el.innerHTML === '') {{ el.innerHTML = words.join(' '); }}
                }}, 6000);

                audio.onloadedmetadata = function() {{
                    var duration = audio.duration * 1000;
                    var speed = Math.max(50, (duration / words.length) - 10);
                    var i = 0;
                    function type() {{
                        if (i < words.length) {{
                            el.innerHTML += (i>0?' ':'') + words[i];
                            i++;
                            setTimeout(type, speed);
                        }}
                    }}
                    type();
                }};
            </script>
        </body>
        </html>
        """
        st.components.v1.html(typing_html, height=220)
    else:
        st.info(f"**{rant_text}**")