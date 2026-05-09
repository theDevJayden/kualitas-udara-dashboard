import streamlit as st
import requests
import pandas as pd
from sklearn.ensemble import IsolationForest
# Import library untuk auto-refresh
from streamlit_autorefresh import st_autorefresh

# ==========================================
# KONFIGURASI THINGSPEAK
# ==========================================
READ_API_KEY = st.secrets["THINGSPEAK_KEY"]
CHANNEL_ID = st.secrets["CHANNEL_ID"]
URL = f"https://api.thingspeak.com/channels/{CHANNEL_ID}/feeds.json?api_key={READ_API_KEY}&results=100"

st.set_page_config(page_title="Dashboard Kualitas Udara", layout="wide")

# Tambahkan Auto-Refresh di sini (20000 ms = 20 detik)  
count = st_autorefresh(interval=20000, limit=None, key="fizzbuzzcounter")

st.title("☁️ Live Air Quality AI based Dashboard")
st.write(f"Data renewed every 20 seconds. (Refresh no-{count})")
st.write(f"Visit my [GitHub : theDevJayden](https://github.com/theDevJayden) for more projects!")

# Hapus ttl=10 atau sesuaikan agar tidak bentrok dengan auto-refresh
@st.cache_data()
def get_thingspeak_data():
    try:
        # Gunakan timeout agar aplikasi tidak stuck jika koneksi lambat
        response = requests.get(URL, timeout=5)
        if response.status_code == 200:
            data = response.json()
            feeds = data.get('feeds', [])
            
            if not feeds:
                return pd.DataFrame()

            df = pd.DataFrame(feeds)
            
            df = df.rename(columns={
                'created_at': 'Waktu',
                'field1': 'MQ-135 (CO2)',
                'field2': 'MQ-135_2 (NO2)',
                'field3': 'MQ-136 (SO2)',
                'field4': 'MQ-131 (O3)',
                'field5': 'MQ-7 (CO)',
                'field6': 'Smoke'
            })
            
            kolom_sensor = ['MQ-135 (CO2)', 'MQ-135_2 (NO2)', 'MQ-136 (SO2)', 'MQ-131 (O3)', 'MQ-7 (CO)', 'Smoke']
            df = df[['Waktu'] + kolom_sensor]
            
            df['Waktu'] = pd.to_datetime(df['Waktu']).dt.tz_convert('Asia/Jakarta')
            df.set_index('Waktu', inplace=True)
            
            for col in kolom_sensor:
                df[col] = pd.to_numeric(df[col], errors='coerce')
                
            return df
        else:
            return None
    except Exception as e:
        st.error(f"Error occurred: {e}")
        return None

def get_status_color(value, safe, warning):
    if value <= safe:
        return "#00FF00"  # Green
    elif value <= warning:
        return "#FFFF00"  # Yellow
    else:
        return "#FF0000"  # Red

def styled_metric(label, value, unit, safe, warning):
    color = get_status_color(value, safe, warning)
    st.markdown(f"""
        <div style="background-color: #1e1e1e; padding: 15px; border-radius: 10px; border-left: 5px solid {color}; margin-bottom: 10px;">
            <p style="margin:0; font-size: 14px; color: #888;">{label}</p>
            <h2 style="margin:0; color: {color};">{value:.2f} <span style="font-size: 16px;">{unit}</span></h2>
        </div>
    """, unsafe_allow_html=True)

# ==========================================
# LOGIKA UPDATE DATA
# ==========================================

# Kita bersihkan cache setiap kali refresh agar data benar-benar baru
st.cache_data.clear()
df = get_thingspeak_data()

if df is not None and not df.empty:
    st.success("Connected to ThingSpeak!")
    
    latest = df.iloc[-1]
    waktu_terakhir = df.index[-1].strftime('%d %B %Y, %H:%M:%S')
    st.caption(f"Last updated: {waktu_terakhir}")
    
    # --- Metrik ---
    # ==========================================
    # 📊 INDIKATOR SAAT INI (WITH THRESHOLDS)
    # ==========================================
    st.markdown("### 📊 Current Air Quality Indicators")
    
    # Define Thresholds: (Safe_Limit, Warning_Limit)
    # Note: Values above Warning_Limit are considered Danger.
    thresholds = {
        "CO2": (400, 1000),    # Outdoor ambient is ~400, Unsafe > 1000
        "SO2": (0.1, 0.5),     # PPM
        "O3": (0.05, 0.1),     # PPM
        "CO": (9.0, 25.0),     # PPM (9 PPM is 8hr limit)
        "Smoke": (50, 150),    # µg/m³
        "NO2": (0.05, 0.2)     # PPM
    }

    col1, col2, col3 = st.columns(3)
    with col1:
        styled_metric("MQ-135 (CO2)", latest['MQ-135 (CO2)'], "PPM", *thresholds["CO2"])
    with col2:
        styled_metric("MQ-136 (SO2)", latest['MQ-136 (SO2)'], "PPM", *thresholds["SO2"])
    with col3:
        styled_metric("MQ-131 (O3)", latest['MQ-131 (O3)'], "PPM", *thresholds["O3"])
    
    st.write("") # Spacer

    col4, col5, col6 = st.columns(3)
    with col4:
        styled_metric("MQ-7 (CO)", latest['MQ-7 (CO)'], "PPM", *thresholds["CO"])
    with col5:
        styled_metric("Smoke", latest['Smoke'], "µg/m³", *thresholds["Smoke"])
    with col6:
        styled_metric("MQ-135 (NO2)", latest['MQ-135_2 (NO2)'], "PPM", *thresholds["NO2"])

    # Legend / Explanation
    st.markdown("""
        <div style="margin-top: 10px; font-size: 12px; text-align: center;">
            <span style="color: #00FF00;">● Green = Safe</span> | 
            <span style="color: #FFFF00;">● Yellow = Warning</span> | 
            <span style="color: #FF0000;">● Red = Danger/Unsafe</span>
        </div>
    """, unsafe_allow_html=True)
    
    st.divider()
    st.markdown("### 📈 Air Quality History")
    st.line_chart(df)
    
    # --- AI Anomaly Detection ---
    st.divider()
    st.markdown("### 🤖 AI Anomaly Detection")
    ml_df = df.dropna().copy()
    
    if len(ml_df) > 10:
        ai_model = IsolationForest(contamination=0.05, random_state=42)
        ml_df['Anomaly'] = ai_model.fit_predict(ml_df)
        anomalies = ml_df[ml_df['Anomaly'] == -1]
        
        if not anomalies.empty:
            st.error(f"⚠️ Warning: Detected {len(anomalies)} anomalies!")
            st.dataframe(anomalies.drop(columns=['Anomaly']))
        else:
            st.success("✅ Air quality is stable.")
    else:
        st.info("Collecting data for AI...")

elif df is not None and df.empty:
    st.warning("No data available from ThingSpeak.")