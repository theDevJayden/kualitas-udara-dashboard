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

# Tambahkan Auto-Refresh di sini (10000 ms = 10 detik)
count = st_autorefresh(interval=10000, limit=None, key="fizzbuzzcounter")

st.title("☁️ Dashboard Pemantauan Kualitas Udara & AI")
st.write(f"Data diperbarui secara otomatis setiap 10 detik. (Refresh ke-{count})")

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
                'field2': 'MQ-135_2',
                'field3': 'MQ-136 (SO2)',
                'field4': 'MQ-131 (O3)',
                'field5': 'MQ-7 (CO)',
                'field6': 'PM 2.5'
            })
            
            kolom_sensor = ['MQ-135 (CO2)', 'MQ-135_2', 'MQ-136 (SO2)', 'MQ-131 (O3)', 'MQ-7 (CO)', 'PM 2.5']
            df = df[['Waktu'] + kolom_sensor]
            
            df['Waktu'] = pd.to_datetime(df['Waktu']).dt.tz_convert('Asia/Jakarta')
            df.set_index('Waktu', inplace=True)
            
            for col in kolom_sensor:
                df[col] = pd.to_numeric(df[col], errors='coerce')
                
            return df
        else:
            return None
    except Exception as e:
        st.error(f"Terjadi kesalahan: {e}")
        return None

# ==========================================
# LOGIKA UPDATE DATA
# ==========================================

# Kita bersihkan cache setiap kali refresh agar data benar-benar baru
st.cache_data.clear()
df = get_thingspeak_data()

if df is not None and not df.empty:
    st.success("Terkoneksi dengan ThingSpeak!")
    
    latest = df.iloc[-1]
    waktu_terakhir = df.index[-1].strftime('%d %B %Y, %H:%M:%S')
    st.caption(f"Pembaruan terakhir: {waktu_terakhir}")
    
    # --- Metrik ---
    col1, col2, col3 = st.columns(3)
    col1.metric("MQ-135 (CO2)", f"{latest['MQ-135 (CO2)']:.2f} PPM")
    col2.metric("MQ-136 (SO2)", f"{latest['MQ-136 (SO2)']:.2f} PPM")
    col3.metric("MQ-131 (O3)", f"{latest['MQ-131 (O3)']:.2f} PPM")
    
    col4, col5, col6 = st.columns(3)
    col4.metric("MQ-7 (CO)", f"{latest['MQ-7 (CO)']:.2f} PPM")
    col5.metric("Sensor PM 2.5", f"{latest['PM 2.5']:.0f} µg/m³")
    col6.metric("MQ-135 (Cadangan)", f"{latest['MQ-135_2']:.2f} PPM")
    
    st.divider()
    st.markdown("### 📈 Grafik Riwayat Kualitas Udara")
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
            st.error(f"⚠️ Peringatan: Terdeteksi {len(anomalies)} anomali!")
            st.dataframe(anomalies.drop(columns=['Anomaly']))
        else:
            st.success("✅ Kualitas udara stabil.")
    else:
        st.info("Mengumpulkan data untuk AI...")

elif df is not None and df.empty:
    st.warning("Belum ada data di ThingSpeak.")