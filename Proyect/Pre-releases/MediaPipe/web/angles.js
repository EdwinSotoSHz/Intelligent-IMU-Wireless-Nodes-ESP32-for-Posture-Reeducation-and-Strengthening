// script-tests-angles.js - Herramientas básicas para ángulos

export class AngleDrawer {
    constructor(ctx) {
        this.ctx = ctx;
    }

    // Ángulo en radianes (0 a 2π)
    angleBetween(A, B, C) {
        const start = Math.atan2(A.y - B.y, A.x - B.x);
        const end = Math.atan2(C.y - B.y, C.x - B.x);
        let angle = end - start;
        if (angle < 0) angle += Math.PI * 2;
        return angle;
    }

    // Ángulo en grados
    getAngleDeg(A, B, C) {
        return this.angleBetween(A, B, C) * (180 / Math.PI);
    }

    // Dibujar líneas BA y BC
    drawLines(A, B, C) {
        this.ctx.beginPath();
        this.ctx.moveTo(B.x, B.y);
        this.ctx.lineTo(A.x, A.y);
        this.ctx.moveTo(B.x, B.y);
        this.ctx.lineTo(C.x, C.y);
        this.ctx.strokeStyle = '#999';
        this.ctx.lineWidth = 2;
        this.ctx.stroke();
    }

    // Dibujar arco del ángulo
    drawAngleArc(A, B, C, radius, opts = {}) {
        const start = Math.atan2(A.y - B.y, A.x - B.x);
        const end = Math.atan2(C.y - B.y, C.x - B.x);
        
        this.ctx.beginPath();
        this.ctx.arc(B.x, B.y, radius, start, end);
        
        if (opts.fill) {
            this.ctx.lineTo(B.x, B.y);
            this.ctx.fillStyle = opts.fill;
            this.ctx.fill();
        }
        
        this.ctx.strokeStyle = opts.stroke || '#000';
        this.ctx.lineWidth = opts.lineWidth || 1.5;
        this.ctx.stroke();
    }

    // Dibujar etiqueta con el valor del ángulo
    drawAngleLabel(A, B, C, radius, opts = {}) {
        const start = Math.atan2(A.y - B.y, A.x - B.x);
        const end = Math.atan2(C.y - B.y, C.x - B.x);
        const mid = (start + end) / 2;
        
        const angle = this.getAngleDeg(A, B, C);
        const label = angle.toFixed(1) + '°';
        const x = B.x + radius * Math.cos(mid);
        const y = B.y + radius * Math.sin(mid);
        
        this.ctx.font = opts.font || '12px monospace';
        this.ctx.fillStyle = opts.color || '#000';
        this.ctx.textAlign = 'center';
        this.ctx.textBaseline = 'middle';
        this.ctx.fillText(label, x, y);
    }

    // Dibujar puntos con etiquetas A, B, C
    drawPoints(A, B, C) {
        const puntos = [
            { p: A, color: '#FF6B6B', label: 'A' },
            { p: B, color: '#4ECDC4', label: 'B' },
            { p: C, color: '#45B7D1', label: 'C' }
        ];
        
        puntos.forEach(({ p, color, label }) => {
            // Círculo exterior
            this.ctx.beginPath();
            this.ctx.arc(p.x, p.y, 8, 0, Math.PI * 2);
            this.ctx.fillStyle = color;
            this.ctx.fill();
            
            // Círculo interior blanco
            this.ctx.beginPath();
            this.ctx.arc(p.x, p.y, 5, 0, Math.PI * 2);
            this.ctx.fillStyle = 'white';
            this.ctx.fill();
            
            // Etiqueta
            this.ctx.fillStyle = '#000';
            this.ctx.font = 'bold 12px monospace';
            this.ctx.textAlign = 'center';
            this.ctx.textBaseline = 'middle';
            this.ctx.fillText(label, p.x, p.y);
        });
    }
}