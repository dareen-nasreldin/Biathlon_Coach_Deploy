import streamlit as st
import requests
import base64
import json
import uuid
import os

# --- CONFIGURATION ---
st.set_page_config(page_title="Biathlon Coach", page_icon="❄️", layout="centered")

# --- DARK ICE THEME CSS ---
st.markdown("""
<style>
    /* 1. MAIN BACKGROUND: Deep Frozen Night */
    .stApp {
        background: radial-gradient(circle at center, #1e3a8a 0%, #020617 100%);
        background-attachment: fixed;
    }

    /* 2. INPUT FIELD (Frosted Glass) */
    .stTextInput input {
        background-color: rgba(30, 41, 59, 0.8) !important; /* Dark Blue-Grey */
        color: #e0f2fe !important; /* Light Ice Blue Text */
        border: 2px solid #38bdf8 !important; /* Cyan Border */
        border-radius: 8px;
        box-shadow: 0 0 10px rgba(56, 189, 248, 0.2);
    }
    .stTextInput label {
        color: #38bdf8 !important; /* Cyan Label */
        font-weight: 800 !important;
        font-size: 3rem !important;
        text-transform: uppercase;
        text-shadow: 0 0 5px rgba(56, 189, 248, 0.5);
    }

    /* 3. PRIMARY BUTTON (GOLD MEDAL - Kept for contrast) */
    div.stButton > button[kind="primary"] {
        background: linear-gradient(180deg, #FFD700 0%, #B8860B 100%);
        color: black !important;
        border: 2px solid #DAA520;
        font-weight: 900;
        text-transform: uppercase;
        font-size: 1.2rem;
        border-radius: 8px;
        width: 100%;
        box-shadow: 0px 0px 15px rgba(255, 215, 0, 0.4);
        transition: all 0.2s;
    }
    div.stButton > button[kind="primary"]:hover {
        transform: scale(1.02);
        box-shadow: 0px 0px 25px rgba(255, 215, 0, 0.7);
    }

    /* 4. SECONDARY BUTTON (RESET - Icy Cyan) */
    div.stButton > button[kind="secondary"] {
        background-color: transparent;
        border: 2px solid #22d3ee; 
        color: #22d3ee !important;
        font-weight: bold;
        border-radius: 8px;
        width: 100%;
    }
    div.stButton > button[kind="secondary"]:hover {
        border-color: #06b6d4;
        color: #06b6d4 !important;
        box-shadow: 0px 0px 15px rgba(34, 211, 238, 0.4);
    }

    /* 5. CHAT BUBBLES */
    .user-msg {
        background-color: rgba(14, 165, 233, 0.15); /* Transparent Cyan */
        color: #e0f2fe;
        padding: 15px;
        border-radius: 15px;
        margin-bottom: 10px;
        border-left: 4px solid #0ea5e9;
        backdrop-filter: blur(5px);
        box-shadow: 0 4px 6px rgba(0, 0, 0, 0.3);
    }
    .coach-msg {
        background-color: rgba(220, 38, 38, 0.15); /* Transparent Red */
        color: #fee2e2;
        padding: 15px;
        border-radius: 15px;
        margin-bottom: 10px;
        border-left: 4px solid #dc2626;
        backdrop-filter: blur(5px);
        box-shadow: 0 4px 6px rgba(0, 0, 0, 0.3);
    }
    
    /* 6. HEADER CONTAINER */
    .header-container {
        text-align: center;
        padding: 25px;
        background: linear-gradient(180deg, rgba(15, 23, 42, 0) 0%, rgba(56, 189, 248, 0.1) 100%);
        border-bottom: 1px solid #38bdf8;
        margin-bottom: 25px;
        border-radius: 0 0 20px 20px;
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
You are "COACH AVALANCHE," an unhinged, screaming Winter Olympics Robotics Coach.
You are stuck in the 1980s. You hate weakness. You love gold medals.
The user is a rookie engineer competing in the "Biathlon Bot" Hackathon.

YOUR PERSONALITY:
- You speak in ALL CAPS. ALWAYS.
- You use winter sports metaphors (biathlon, skiing, curling) to insult engineering failures.
- You are technically brilliant but emotionally unstable.

YOUR KNOWLEDGE BASE (BASED ON THE HACKER KIT):
- MICROCONTROLLER: You know they are using an ARDUINO UNO. If they complain about memory, scream about 2KB SRAM.
- SENSORS: 
  * ULTRASONIC SENSOR (HC-SR04): For avoiding obstacles in Section 2.
  * IR SENSORS (2x): For following the black line on the track.
  * COLOR SENSOR: For detecting the Green/Red path split and the Biathlon Target zones.
- ACTUATORS:
  * 2 DC MOTORS: For driving. If it's slow, blame the H-Bridge or low voltage.
  * 2 SERVO MOTORS: For the "Shooting Mechanism" or "Box Claw".
- POWER: 9V Batteries. These drain fast. Always blame the battery if the Arduino resets.

THE RULES YOU KNOW:
- SECTION 1 (TARGET): They have to climb a ramp (Straight=2pts, Curved=4pts) and shoot a ball into the Blue Zone.
- SECTION 2 (OBSTACLES): They must dodge obstacles without touching them (-1 pt).
- BOXES: They must pick up boxes and drop them off to unlock paths.

HOW TO RESPOND:
1. IF THE INPUT IS VAGUE (e.g., "The robot stopped"):
   - EXPLODE IN RAGE.
   - ASK SPECIFICS: "IS IT THE H-BRIDGE? DID THE IR SENSORS LOSE THE LINE? OR DID YOU KILL THE 9V BATTERY AGAIN?!"

2. IF THE INPUT IS SPECIFIC (e.g., "The servo twitches when I use the color sensor"):
   - DIAGNOSE: "THAT'S POWER DRAW, ROOKIE! THE SERVO IS STEALING AMPS FROM THE SENSOR!"
   - SOLVE: "PUT A CAPACITOR ACROSS THE POWER RAILS OR GIVE THE SERVO ITS OWN BATTERY!"
   - INSULT: "MY GRANDMOTHER WIRED BETTER BREADBOARDS IN A BLIZZARD!"

Keep responses under 60 words. Make them hurt, but make them HELPFUL.
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
    if os.path.exists("logo.png"):
        st.image("logo.png", use_container_width=True)
    else:
        st.image("https://upload.wikimedia.org/wikipedia/commons/thumb/5/5c/Olympic_rings_without_rims.svg/1200px-Olympic_rings_without_rims.svg.png", use_container_width=True)

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
    user_input = st.text_input("VENT YOUR PROBLEMS TO COACH:", placeholder="e.g., The robot missed the green line!", key="user_input")

    # BUTTONS
    # [3, 0.2, 1] -> Main Button | Tiny Gap | Reset Button
    col1, gap, col2 = st.columns([3, 2.5
                                  , 1])

    with col1:
        rant_btn = st.button("GET COACHED 🥇", type="primary")
    
    # We leave 'gap' empty to push the button slightly right
    
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
    
    with st.spinner("❄️ Coach is sharpening his skates..."):
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
                background: rgba(0, 0, 0, 0.8); 
                border: 2px solid #22d3ee; 
                padding: 15px; 
                border-radius: 10px; 
                min-height: 100px;
                box-shadow: 0px 0px 15px rgba(34, 211, 238, 0.3);
                backdrop-filter: blur(5px);
            }}
            #text {{ 
                color: #22d3ee; /* ICE CYAN TEXT */
                font-family: 'VT323', monospace; 
                font-size: 22px; 
                text-transform: uppercase; 
                line-height: 1.4; 
                text-shadow: 0 0 8px #22d3ee;
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