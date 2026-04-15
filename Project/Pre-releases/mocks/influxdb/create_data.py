import numpy as np
import pandas as pd
from datetime import datetime, timedelta

def generate_realistic_data(days=5, freq_sec=4):
    """
    Genera datos IMU/ECG realistas con patrones circadianos y ruido.
    
    Args:
        days: número de días a generar
        freq_sec: frecuencia en segundos (3-5 recomendado)
    """
    end_date = datetime(2026, 4, 7, 0, 0, 0) + timedelta(days=days)
    timestamps = pd.date_range(
        start=datetime(2026, 4, 7, 0, 0, 0),
        end=end_date,
        freq=f'{freq_sec}s'
    )
    
    print(f"Generando {len(timestamps):,} timestamps...")
    
    fields_config = {
        'ecg': {'range': (1500, 3500), 'noise': 80, 'daily_amp': 400},
        'pitchA': {'range': (-30, 30), 'noise': 1.5, 'daily_amp': 10},
        'pitchF': {'range': (-30, 30), 'noise': 1.5, 'daily_amp': 10},
        'rollA': {'range': (-180, 180), 'noise': 2, 'daily_amp': 30},
        'rollF': {'range': (-180, 180), 'noise': 2, 'daily_amp': 30},
        'yawA': {'range': (-180, 180), 'noise': 2.5, 'daily_amp': 45},
        'yawF': {'range': (-180, 180), 'noise': 2.5, 'daily_amp': 45},
    }
    
    all_rows = []
    
    for ts in timestamps:
        hour = ts.hour
        minute = ts.minute
        second = ts.second
        
        # Patrón circadiano (más actividad de día)
        circadian = 0.5 + 0.5 * np.sin((hour - 12) * np.pi / 12)
        
        for field, config in fields_config.items():
            # Generar valor base con patrón diario
            if field == 'ecg':
                # ECG: más alto durante actividad diurna
                base = 2000 + config['daily_amp'] * circadian
                # Añadir ritmo cardiaco (variaciones de ~1Hz)
                base += 50 * np.sin(ts.timestamp() * 2 * np.pi)
            else:
                # IMU: movimientos durante el día, quieto de noche
                activity = circadian * config['daily_amp']
                base = activity * np.sin(ts.timestamp() / 300)
                
                # Añadir pequeñas variaciones
                base += config['noise'] * np.random.randn()
            
            # Añadir ruido realista
            value = base + config['noise'] * np.random.randn()
            
            # Limitar a rangos realistas
            value = np.clip(value, config['range'][0], config['range'][1])
            
            # Ocasionalmente añadir artefactos (picos)
            if np.random.random() < 0.001:  # 0.1% de los datos
                value *= np.random.uniform(1.5, 3.0)
                value = np.clip(value, config['range'][0], config['range'][1])
            
            all_rows.append({
                'result': '',
                'table': list(fields_config.keys()).index(field),
                '_start': '2026-04-07T15:31:55.354161884Z',
                '_stop': '2026-04-14T15:31:55.354161884Z',
                '_time': ts.strftime('%Y-%m-%dT%H:%M:%SZ'),
                '_value': round(value, 6),
                '_field': field,
                '_measurement': 'prototype_data'
            })
    
    # Crear DataFrame y ordenar
    df = pd.DataFrame(all_rows)
    df = df.sort_values(['_time', 'table'])
    
    return df

# Generar datos
df = generate_realistic_data(days=5, freq_sec=4)

# Guardar
output_file = 'datos_imu_ecg_5dias.csv'
with open(output_file, 'w') as f:
    f.write('#group,false,false,true,true,false,false,true,true\n')
    f.write('#datatype,string,long,dateTime:RFC3339,dateTime:RFC3339,dateTime:RFC3339,double,string,string\n')
    f.write('#default,mean,,,,,,,\n')
    df.to_csv(f, index=False)

print(f"\n✅ Generado: {output_file}")
print(f"   - Días: 5")
print(f"   - Frecuencia: cada 4 segundos")
print(f"   - Filas: {len(df):,}")
print(f"   - Tamaño: ~{len(df) * 100 / 1024 / 1024:.1f} MB")