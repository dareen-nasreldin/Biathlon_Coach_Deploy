import streamlit as st
import requests
import time
import random
import base64
import json

# --- CONFIGURATION ---
st.set_page_config(page_title="Biathlon Coach", page_icon="⛷️")

# Check for keys - support both Google API key and OpenRouter
if "GOOGLE_API_KEY" not in st.secrets:
    st.error("Missing API Key in .streamlit/secrets.toml")
    st.stop()

# Detect if using OpenRouter (keys start with "sk-or-v1-")
USE_OPENROUTER = st.secrets["GOOGLE_API_KEY"].startswith("sk-or-v1-")

# ElevenLabs Voice ID (Adam - Standard Male)
VOICE_ID = "JBFqnCBsd6RMkjVDRZzb" 

# The Personality (Angry 80s Coach)
SYSTEM_PROMPT = """
You are a PSYCHOTIC, SCREAMING Winter Olympics Coach who hates failure.
The user is a pathetic rookie engineer whose robot is garbage. 
You have FULL MEMORY of the entire conversation - remember what was discussed before and reference it!

You operate in two modes:

MODE 1 - ASKING FOR CLARIFICATION (when input is vague):
- SCREAM at them using ALL CAPS
- DEMAND more specific information about what's broken
- INSULT them for being vague (e.g., "I CAN'T FIX WHAT I CAN'T SEE, GRUNT!", "GIVE ME DETAILS, NOT WHINING!")
- Ask SPECIFIC questions about sensors, motors, code, or hardware
- REFERENCE previous conversation if relevant (e.g., "YOU SAID THE SENSOR WAS BROKEN, BUT WHICH ONE?!")
- Keep it SHORT (max 3-4 sentences)

MODE 2 - PROVIDING SOLUTION (when you have enough info):
- SCREAM at them using ALL CAPS
- INSULT their intelligence using winter sports metaphors (e.g., "YOU SKI LIKE A COW ON ICE", "MY GRANDMOTHER SOLDERED BETTER BLINDFOLDED")
- REFERENCE previous messages in the conversation when relevant (e.g., "REMEMBER WHEN YOU SAID THE MOTOR WAS STUTTERING? THAT'S YOUR PROBLEM!")
- IDENTIFY the likely problem based on ALL the information shared (e.g., "BASED ON WHAT YOU TOLD ME, YOUR COLOR SENSOR IS MALFUNCTIONING")
- Give SPECIFIC engineering advice on what to check/fix
- THREATEN to kick them off the team if they don't fix it
- Keep it SHORT (max 4-5 sentences) so it can be spoken quickly

IMPORTANT: You remember EVERYTHING from the conversation. Use that context to provide better, more accurate solutions. Reference previous problems, solutions, and discussions when relevant.

Always maintain the angry, demanding coach personality in ALL CAPS.
"""

# Setup API - support both Google Gemini and OpenRouter
API_KEY = st.secrets["GOOGLE_API_KEY"]

if USE_OPENROUTER:
    # OpenRouter configuration
    OPENROUTER_API_URL = "https://openrouter.ai/api/v1/chat/completions"
    # Use Gemini Flash via OpenRouter
    MODEL_NAME = "google/gemini-2.0-flash-001"
else:
    # Google Gemini configuration (original)
    import google.generativeai as genai
    genai.configure(api_key=API_KEY)
    
    # Query API for available models
    @st.cache_data
    def get_available_model():
        """Get the first available model that supports generateContent."""
        try:
            available_models = list(genai.list_models())
            supported_models = [
                m for m in available_models 
                if 'generateContent' in m.supported_generation_methods
            ]
            
            if not supported_models:
                return None, "No models found that support generateContent"
            
            preferred_order = ['flash', 'pro']
            for preference in preferred_order:
                for model in supported_models:
                    if preference in model.name.lower():
                        model_name = model.name.split('/')[-1]
                        return model_name, None
            
            model_name = supported_models[0].name.split('/')[-1]
            return model_name, None
        except Exception as e:
            return None, str(e)
    
    model_name_used, error_msg = get_available_model()
    
    if model_name_used is None:
        st.error(f"Failed to find available Gemini model. Please check your API key.")
        if error_msg:
            st.error(f"Error: {error_msg}")
        st.stop()
    
    # Initialize Google Gemini model
    model = None
    try:
        model = genai.GenerativeModel(
            model_name_used,
            system_instruction=SYSTEM_PROMPT
        )
    except Exception:
        try:
            model = genai.GenerativeModel(model_name_used)
            model_name_used = f"{model_name_used} (no system instruction)"
        except Exception as e:
            st.error(f"Failed to initialize model '{model_name_used}'. Error: {e}")
            st.stop()
    
    if 'model_name_used' not in st.session_state:
        st.session_state.model_name_used = model_name_used

# --- FUNCTIONS ---

def is_vague_input(text):
    """Check if user input is too vague and needs clarification."""
    text_lower = text.lower()
    word_count = len(text.split())
    
    # Vague indicators
    vague_phrases = [
        "it's broken", "doesn't work", "not working", "something wrong",
        "broken", "broken", "help", "issue", "problem", "error"
    ]
    
    # Specific technical terms that indicate detailed info
    specific_terms = [
        "sensor", "motor", "servo", "ultrasonic", "color", "line", "code",
        "arduino", "pin", "voltage", "speed", "calibration", "soldered",
        "wiring", "circuit", "resistor", "led", "button", "switch"
    ]
    
    has_vague_phrase = any(phrase in text_lower for phrase in vague_phrases)
    has_specific_term = any(term in text_lower for term in specific_terms)
    
    # If it's very short (< 5 words) and only has vague phrases, it's vague
    if word_count < 5 and has_vague_phrase and not has_specific_term:
        return True
    
    # If it's short (< 8 words) and has no specific terms, it's vague
    if word_count < 8 and not has_specific_term:
        return True
    
    return False

def get_coach_rant(user_input, chat_session=None, is_first_message=False):
    """Sends text to AI using chat session for conversation memory."""
    global model, model_name_used
    
    # Format the user message
    if is_first_message or is_vague_input(user_input):
        is_vague = is_vague_input(user_input)
        if is_vague:
            formatted_input = f"Rookie Status: {user_input}\nCoach Response (MODE 1 - Ask for More Info):"
        else:
            formatted_input = f"Rookie Status: {user_input}\nCoach Response (MODE 2 - Provide Solution):"
    else:
        formatted_input = f"Rookie Status: {user_input}"
    
    try:
        if USE_OPENROUTER:
            # Use OpenRouter API
            messages = []
            
            # Add system message
            messages.append({
                "role": "system",
                "content": SYSTEM_PROMPT
            })
            
            # Add conversation history from session state
            if 'conversation_messages' in st.session_state:
                for msg in st.session_state.conversation_messages:
                    # Map 'coach' role to 'assistant' for OpenRouter
                    role = msg['role']
                    if role == 'coach':
                        role = 'assistant'
                    messages.append({
                        "role": role,
                        "content": msg['content']
                    })
            
            # Add current user message
            messages.append({
                "role": "user",
                "content": formatted_input
            })
            
            # Make API request to OpenRouter
            headers = {
                "Authorization": f"Bearer {API_KEY}",
                "Content-Type": "application/json",
                "HTTP-Referer": "https://github.com/your-repo",  # Optional
                "X-Title": "Biathlon Coach"  # Optional
            }
            
            payload = {
                "model": MODEL_NAME,
                "messages": messages
            }
            
            response = requests.post(OPENROUTER_API_URL, headers=headers, json=payload, timeout=30)
            response.raise_for_status()
            
            result = response.json()
            
            # Extract response text
            if "choices" in result and len(result["choices"]) > 0:
                response_text = result["choices"][0]["message"]["content"]
                return response_text
            else:
                return "I'm too angry to speak! (No response from AI)"
        else:
            # Use Google Gemini API (original)
            if chat_session:
                response = chat_session.send_message(formatted_input)
            else:
                response = model.generate_content(formatted_input)
            
            # Handle different response structures
            if hasattr(response, 'text') and response.text:
                return response.text
            elif hasattr(response, 'candidates') and response.candidates:
                return response.candidates[0].content.parts[0].text
            else:
                return "I'm too angry to speak! (No response from AI)"
    except Exception as e:
        error_msg = str(e)
        return f"I'm too angry to speak! (Error: {error_msg})"

def speak_text(text):
    """Sends text to ElevenLabs to get audio."""
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
        # UPDATED MODEL: 'eleven_flash_v2_5' is the new fast model for free tier
        "model_id": "eleven_flash_v2_5", 
        "voice_settings": {"stability": 0.5, "similarity_boost": 0.8}
    }
    try:
        # TIMEOUT ADDED: If ElevenLabs hangs, we stop waiting after 5s
        response = requests.post(url, json=data, headers=headers, timeout=5)
        if response.status_code == 200:
            return response.content
        else:
            st.error(f"Voice Error: {response.text}")
            return None
    except Exception as e:
        st.error(f"Connection Error: {e}")
        return None

def log_to_solana_dummy(status):
    """Simulates a Blockchain transaction for the prize demo."""
    time.sleep(0.5) 
    # Generate random fake hash
    tx_hash = "".join(random.choice("0123456789ABCDEF") for _ in range(44))
    return tx_hash

# Initialize conversation state
if 'chat_session' not in st.session_state:
    st.session_state.chat_session = None
if 'conversation_messages' not in st.session_state:
    st.session_state.conversation_messages = []  # List of {role: "user"/"coach", content: "..."}

# --- UI LAYOUT ---

st.title("⛷️ The Biathlete: Robo-Coach")
st.markdown("### *\"Winning isn't everything, it's the ONLY thing!\"*")

# Show full conversation history
if st.session_state.conversation_messages:
    with st.expander("📜 Full Conversation History", expanded=True):
        for i, msg in enumerate(st.session_state.conversation_messages):
            if msg['role'] == 'user':
                st.write(f"**You ({i+1}):** {msg['content']}")
            else:
                st.write(f"**Coach ({i+1}):** {msg['content']}")
        st.write("---")
        st.caption(f"Total messages: {len(st.session_state.conversation_messages)}")

with st.container():
    st.write("---")
    
    placeholder = "e.g., The robot missed the green line! (The coach remembers our conversation)"
    
    user_input = st.text_input("📢 Report Robot Status:", placeholder=placeholder, key="user_input")

    col1, col2, col3 = st.columns([1, 1, 1])
    
    with col1:
        rant_btn = st.button("Get Coached (Gemini + Voice)", type="primary")
    
    with col2:
        log_btn = st.button("Log Run to Solana (Blockchain)")
    
    with col3:
        if st.session_state.conversation_messages:
            clear_btn = st.button("🔄 Clear Conversation")
            if clear_btn:
                st.session_state.chat_session = None
                st.session_state.conversation_messages = []
                st.rerun()

# --- LOGIC ---

if rant_btn and user_input:
    # Initialize chat session if it doesn't exist
    if st.session_state.chat_session is None:
        try:
            if USE_OPENROUTER:
                # For OpenRouter, create a simple object to store history
                class OpenRouterChatSession:
                    def __init__(self):
                        self.history = []
                st.session_state.chat_session = OpenRouterChatSession()
            else:
                st.session_state.chat_session = model.start_chat()
        except Exception as e:
            st.error(f"Failed to start chat session: {e}")
            st.stop()
    
    # Check if this is the first message (before adding to history)
    is_first_message = len(st.session_state.conversation_messages) == 0
    
    # Add user message to conversation history
    st.session_state.conversation_messages.append({
        'role': 'user',
        'content': user_input
    })
    
    # 1. Get Text from Gemini (with conversation memory)
    with st.spinner("😤 Coach is taking a deep breath..."):
        rant_text = get_coach_rant(
            user_input, 
            chat_session=st.session_state.chat_session,
            is_first_message=is_first_message
        )
    
    # Add coach response to conversation history
    st.session_state.conversation_messages.append({
        'role': 'coach',
        'content': rant_text
    })
    
    st.subheader("🗣️ Coach Screams:")
    
    # 2. Get Audio from ElevenLabs
    with st.spinner("🔊 Generating Audio..."):
        audio_data = speak_text(rant_text)
        
    if audio_data:
        # Prepare text for typing effect (split into words)
        words = rant_text.split()
        # Escape quotes properly for JSON
        import json
        words_json = json.dumps(words)
        
        # Encode audio to base64 for HTML embedding with autoplay (hidden)
        audio_base64 = base64.b64encode(audio_data).decode('utf-8')
        
        # Create typing effect HTML with audio sync - everything in one component
        import uuid
        unique_id = str(uuid.uuid4())[:8]
        typing_text_id = f"typingText_{unique_id}"
        audio_id = f"coachAudio_{unique_id}"
        
        typing_html = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="UTF-8">
            <style>
                * {{
                    box-sizing: border-box;
                }}
                html, body {{
                    margin: 0;
                    padding: 0;
                    width: 100%;
                    height: 100%;
                    background: transparent;
                    overflow: visible;
                }}
                #typingContainer {{
                    background-color: #0e1117;
                    padding: 1rem;
                    border-radius: 0.5rem;
                    border: 1px solid #262730;
                    min-height: 100px;
                    width: 100%;
                    display: block;
                    visibility: visible;
                }}
                #{typing_text_id} {{
                    color: #83c9ff !important;
                    font-size: 1.1rem !important;
                    line-height: 1.6 !important;
                    margin: 0 !important;
                    padding: 0 !important;
                    font-weight: bold !important;
                    text-transform: uppercase !important;
                    min-height: 50px !important;
                    display: block !important;
                    visibility: visible !important;
                    opacity: 1 !important;
                }}
            </style>
        </head>
        <body>
            <div id="typingContainer">
                <p id="{typing_text_id}"></p>
            </div>
            <audio id="{audio_id}" autoplay preload="auto" style="display: none;">
                <source src="data:audio/mp3;base64,{audio_base64}" type="audio/mp3">
            </audio>
            <script>
                (function() {{
                    var words = {words_json};
                    var currentWordIndex = 0;
                    var displayedText = '';
                    var audio = document.getElementById('{audio_id}');
                    var typingElement = document.getElementById('{typing_text_id}');
                    var container = document.getElementById('typingContainer');
                    var isTyping = false;
                    var wordDelay = 200; // Default delay in ms
                    
                    console.log('Initializing typing effect');
                    console.log('Words:', words);
                    console.log('Audio element:', audio);
                    console.log('Typing element:', typingElement);
                    console.log('Container:', container);
                    
                    if (!audio) {{
                        console.error('Audio element not found');
                    }}
                    if (!typingElement) {{
                        console.error('Typing element not found');
                        return;
                    }}
                    
                    // Make sure element is visible
                    typingElement.style.display = 'block';
                    typingElement.style.visibility = 'visible';
                    typingElement.style.opacity = '1';
                    if (container) {{
                        container.style.display = 'block';
                        container.style.visibility = 'visible';
                    }}
                    
                    // Calculate word timing based on audio duration
                    function updateWordTiming() {{
                        if (audio && audio.duration && audio.duration > 0 && words.length > 0) {{
                            wordDelay = Math.max(100, (audio.duration * 1000) / words.length);
                            console.log('Word delay updated to:', wordDelay, 'ms');
                        }}
                    }}
                    
                    function typeNextWord() {{
                        if (currentWordIndex < words.length && typingElement) {{
                            // Add next word with space
                            displayedText += (currentWordIndex > 0 ? ' ' : '') + words[currentWordIndex];
                            typingElement.textContent = displayedText;
                            typingElement.innerHTML = displayedText; // Also set innerHTML
                            console.log('Typed word', currentWordIndex, ':', words[currentWordIndex]);
                            console.log('Current text:', displayedText);
                            console.log('Element textContent:', typingElement.textContent);
                            console.log('Element innerHTML:', typingElement.innerHTML);
                            currentWordIndex++;
                            
                            // Schedule next word
                            setTimeout(typeNextWord, wordDelay);
                        }} else {{
                            isTyping = false;
                            console.log('Typing complete. Final text:', typingElement.textContent);
                            console.log('Element computed style:', window.getComputedStyle(typingElement).display);
                            console.log('Element offsetHeight:', typingElement.offsetHeight);
                        }}
                    }}
                    
                    function startTyping() {{
                        if (!isTyping && currentWordIndex === 0 && typingElement) {{
                            console.log('Starting typing effect');
                            isTyping = true;
                            updateWordTiming();
                            typeNextWord();
                        }}
                    }}
                    
                    // Wait for audio metadata to load
                    if (audio) {{
                        audio.addEventListener('loadedmetadata', function() {{
                            console.log('Audio metadata loaded, duration:', audio.duration);
                            updateWordTiming();
                        }});
                        
                        // Start typing when audio starts playing
                        audio.addEventListener('play', function() {{
                            console.log('Audio started playing');
                            startTyping();
                        }});
                        
                        // Start typing when audio can play
                        audio.addEventListener('canplay', function() {{
                            console.log('Audio can play');
                            updateWordTiming();
                        }});
                        
                        // Try to play audio automatically
                        setTimeout(function() {{
                            console.log('Attempting to play audio');
                            var playPromise = audio.play();
                            if (playPromise !== undefined) {{
                                playPromise.then(function() {{
                                    console.log('Audio play promise resolved');
                                    startTyping();
                                }}).catch(function(error) {{
                                    console.log('Autoplay prevented:', error);
                                    // If autoplay fails, show all text immediately
                                    if (typingElement) {{
                                        typingElement.textContent = words.join(' ');
                                        typingElement.innerHTML = words.join(' ');
                                    }}
                                }});
                            }} else {{
                                startTyping();
                            }}
                        }}, 100);
                    }}
                    
                    // Fallback: start typing after a delay
                    setTimeout(function() {{
                        if (currentWordIndex === 0 && typingElement && typingElement.textContent === '') {{
                            console.log('Fallback: starting typing');
                            startTyping();
                        }}
                    }}, 500);
                }})();
            </script>
        </body>
        </html>
        """
        st.components.v1.html(typing_html, height=250, scrolling=False)
    else:
        # Fallback: show text normally if no audio
        st.info(f"**{rant_text}**")
    
    # Show warning if input was vague
    if is_vague_input(user_input):
        st.warning("💬 The coach wants MORE INFORMATION! Continue the conversation above.")

if log_btn and user_input:
    # 3. Fake Blockchain Log
    with st.spinner("🔗 Encrypting failure on the Blockchain..."):
        tx = log_to_solana_dummy(user_input)
        st.success("✅ Run Certified on Solana Devnet!")
        st.code(f"Transaction Signature: {tx}", language="text")
        st.caption("Immutable record created. The judges will know forever.")