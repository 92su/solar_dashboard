import streamlit as st
import requests
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime, timedelta
from fpdf import FPDF

# --- Flask endpoint ---
FLASK_URL = "http://192.168.100.99:5000/latest"

st.set_page_config(page_title="Solar & Battery Dashboard", layout="wide")
st.title("Solar & Battery Monitoring Dashboard")

# --- Session state to store historical data ---
if "history" not in st.session_state:
    st.session_state.history = []

# --- Fetch latest data from Flask ---
def fetch_data():
    try:
        resp = requests.get(FLASK_URL, timeout=5)
        if resp.status_code == 200:
            return resp.json()
    except:
        return {}
    return {}

# --- Top Refresh button ---
if st.button("🔄 Refresh Data (Top)"):
    data = fetch_data()
    if data:
        st.session_state.history.append(data)
    else:
        st.warning("No data received from ESP32/Flask.")

# --- Flatten JSON into rows for DataFrame ---
def flatten_data(d):
    rows = []
    ts = d.get('timestamp', datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
    for sensor in ['ds18b20', 'g1', 'g2']:
        if sensor in d:
            for k, v in d[sensor].items():
                rows.append({
                    'Timestamp': ts,
                    'Section': sensor,
                    'Label': k,
                    'Value': v,
                    'Value_with_units': f"{v}"
                })
    return rows

# --- Build DataFrame from history ---
flat_list = []
for d in st.session_state.history:
    flat_list.extend(flatten_data(d))

df = pd.DataFrame(flat_list)
if df.empty:
    st.info("Press 'Refresh Data' to fetch ESP32 data.")
    st.stop()

# --- Convert Timestamp to Python datetime ---
df['Timestamp'] = pd.to_datetime(df['Timestamp'])
min_time = df['Timestamp'].min().to_pydatetime()
max_time = df['Timestamp'].max().to_pydatetime()

# Add 1 second if min==max to avoid slider crash
if min_time == max_time:
    max_time += timedelta(seconds=1)

# --- Sidebar filters ---
st.sidebar.title("🔍 Filters")
section_filter = st.sidebar.multiselect("Select Section", df['Section'].unique(), default=df['Section'].unique())
label_filter = st.sidebar.multiselect("Select Label", df['Label'].unique(), default=df['Label'].unique())

time_range = st.sidebar.slider(
    "Time Range",
    min_value=min_time,
    max_value=max_time,
    value=(min_time, max_time),
    format="YYYY-MM-DD HH:mm:ss"
)

# --- Filter DataFrame ---
filtered = df[
    (df['Section'].isin(section_filter)) &
    (df['Label'].isin(label_filter)) &
    (df['Timestamp'].between(time_range[0], time_range[1]))
]

# === KPI Cards for Latest Values ===
st.subheader("📊 Live Sensor Data")
latest = df[df['Timestamp'] == df['Timestamp'].max()]

col1, col2, col3, col4 = st.columns(4)
with col1:
    vbat = latest[latest['Label'].str.contains("batVoltage", case=False)]['Value']
    if not vbat.empty:
        st.metric("🔋 Battery Voltage", f"{vbat.iloc[0]:.2f} V")

with col2:
    curr = latest[latest['Label'].str.contains("Current", case=False)]['Value']
    if not curr.empty:
        st.metric("⚡ Current", f"{curr.iloc[0]:.2f} A")

with col3:
    temp = latest[latest['Label'].str.contains("Temp", case=False)]['Value']
    if not temp.empty:
        st.metric("🌡️ Temperature", f"{temp.iloc[0]:.2f} °C")

with col4:
    soc = latest[latest['Label'].str.contains("batterySoC", case=False)]['Value']
    if not soc.empty:
        st.metric("📈 Battery SoC", f"{soc.iloc[0]:.1f} %")

# --- Fault/Warning/Status Summary ---
st.subheader("⚠️ Fault, Warning & Status Code Summary")
status_df = filtered[filtered['Label'].str.lower().str.contains('status')]
warning_df = filtered[filtered['Label'].str.lower().str.contains('warn')]
fault_df = filtered[filtered['Label'].str.lower().str.contains('fault')]

col1, col2, col3 = st.columns(3)
col1.markdown(f"<div style='background-color:#e3f2fd; padding:20px; border-radius:15px; text-align:center;'>"
              f"<h3 style='color:#1565c0;'>🔵 Status</h3><h2>{len(status_df)}</h2></div>", unsafe_allow_html=True)
col2.markdown(f"<div style='background-color:#fff3cd; padding:20px; border-radius:15px; text-align:center;'>"
              f"<h3 style='color:#f0ad4e;'>🟡 Warnings</h3><h2>{len(warning_df)}</h2></div>", unsafe_allow_html=True)
col3.markdown(f"<div style='background-color:#f8d7da; padding:20px; border-radius:15px; text-align:center;'>"
              f"<h3 style='color:#d9534f;'>🔴 Faults</h3><h2>{len(fault_df)}</h2></div>", unsafe_allow_html=True)

# --- Time Series Plots ---
st.subheader("📈 Time Series Plots")
cols = st.columns(2)
for i, label in enumerate(label_filter):
    plot_data = filtered[filtered['Label'] == label]
    if not plot_data.empty:
        fig, ax = plt.subplots(figsize=(6,3))
        for section in plot_data['Section'].unique():
            section_data = plot_data[plot_data['Section'] == section]
            ax.plot(section_data['Timestamp'], section_data['Value'], label=section, marker='o')
        ax.set_title(label)
        ax.set_xlabel("Time")
        ax.set_ylabel(label)
        ax.grid(True)
        ax.legend()
        cols[i % 2].pyplot(fig)

# --- Show filtered table ---
with st.expander("📋 Show Filtered Table"):
    st.dataframe(filtered[['Timestamp', 'Section', 'Label', 'Value_with_units']])

# --- Show full history table ---
st.subheader("🗂️ Full History Data")
st.dataframe(df[['Timestamp', 'Section', 'Label', 'Value_with_units']])

# --- Download full history as CSV ---
csv = df.to_csv(index=False).encode('utf-8')
st.download_button(
    label="💾 Download Full History as CSV",
    data=csv,
    file_name="history_data.csv",
    mime="text/csv"
)

# --- Bottom Refresh button ---
st.markdown("---")
if st.button("🔄 Refresh Data (Bottom)"):
    data = fetch_data()
    if data:
        st.session_state.history.append(data)
        st.rerun()
    else:
        st.warning("No data received from ESP32/Flask.")
