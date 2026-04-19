import numpy as np
import pandas as pd
from datetime import datetime, timedelta

def generate_realistic_data(specific_dates=None, freq_sec=6):
    """
    Genera datos IMU/ECG realistas con patrones circadianos y ruido.
    """
    if specific_dates is None:
        specific_dates = ['2026-03-30','2026-04-02', '2026-04-07', '2026-04-09', 
                          '2026-04-11', '2026-04-12', '2026-04-15', '2026-04-17']
    
    # Generar todos los timestamps para las fechas específicas
    all_timestamps = []
    for date_str in specific_dates:
        start_date = datetime.strptime(date_str, '%Y-%m-%d')
        end_date = start_date + timedelta(days=1) - timedelta(seconds=freq_sec)
        
        timestamps = pd.date_range(
            start=start_date,
            end=end_date,
            freq=f'{freq_sec}s'
        )
        all_timestamps.extend(timestamps)
    
    all_timestamps = sorted(all_timestamps)
    
    print(f"Generando {len(all_timestamps):,} timestamps...")
    print(f"Fechas incluidas: {specific_dates}")
    
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
    
    for ts in all_timestamps:
        hour = ts.hour
        
        # Patrón circadiano (más actividad de día)
        circadian = 0.5 + 0.5 * np.sin((hour - 12) * np.pi / 12)
        
        for idx, (field, config) in enumerate(fields_config.items()):
            # Generar valor base con patrón diario
            if field == 'ecg':
                base = 2000 + config['daily_amp'] * circadian
                base += 50 * np.sin(ts.timestamp() * 2 * np.pi)
            else:
                activity = circadian * config['daily_amp']
                base = activity * np.sin(ts.timestamp() / 300)
                base += config['noise'] * np.random.randn()
            
            # Añadir ruido realista
            value = base + config['noise'] * np.random.randn()
            
            # Limitar a rangos realistas
            value = np.clip(value, config['range'][0], config['range'][1])
            
            # Ocasionalmente añadir artefactos (picos)
            if np.random.random() < 0.001:
                value *= np.random.uniform(1.5, 3.0)
                value = np.clip(value, config['range'][0], config['range'][1])
            
            all_rows.append({
                'result': '',
                'table': idx,
                '_start': '2026-04-19T20:36:50.73357964Z',  # Usando el mismo que en query(2).csv
                '_stop': '2026-04-19T21:36:50.73357964Z',
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
fechas_especificas = ['2026-04-02', '2026-04-07', '2026-04-09', '2026-04-11', '2026-04-12', '2026-04-15']
df = generate_realistic_data(specific_dates=fechas_especificas, freq_sec=6)

# Guardar con formato IDÉNTICO a query(2).csv
output_file = 'data.csv'
with open(output_file, 'w') as f:
    # Escribir encabezados
    f.write('#group,false,false,true,true,false,false,true,true\n')
    f.write('#datatype,string,long,dateTime:RFC3339,dateTime:RFC3339,dateTime:RFC3339,double,string,string\n')
    f.write('#default,mean,,,,,,,\n')
    
    # Escribir encabezado de columnas (importante: empieza con coma)
    f.write(',result,table,_start,_stop,_time,_value,_field,_measurement\n')
    
    # Escribir datos: CADA LÍNEMA EMPIEZA CON UNA SOLA COMA
    for _, row in df.iterrows():
        # Formato: ,{result},{table},{_start},{_stop},{_time},{_value},{_field},{_measurement}
        line = f",{row['result']},{row['table']},{row['_start']},{row['_stop']},{row['_time']},{row['_value']},{row['_field']},{row['_measurement']}\n"
        f.write(line)

print(f"\n✅ Generado: {output_file}")
print(f"   - Fechas: {len(fechas_especificas)} días específicos")
print(f"   - Frecuencia: cada 6 segundos")
print(f"   - Filas: {len(df):,}")