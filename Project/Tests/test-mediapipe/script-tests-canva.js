// script-tests-canva.js - Generador simple de puntos aleatorios

export class RandomPointGenerator {
    constructor(width, height, margin = 50) {
        this.width = width;
        this.height = height;
        this.margin = margin;
    }

    // Generar un punto aleatorio
    randomPoint() {
        return {
            x: this.margin + Math.random() * (this.width - this.margin * 2),
            y: this.margin + Math.random() * (this.height - this.margin * 2)
        };
    }

    // Generar 3 puntos aleatorios
    generateThreePoints() {
        return {
            A: this.randomPoint(),
            B: this.randomPoint(),
            C: this.randomPoint()
        };
    }
}

// Función helper para uso rápido
export function getRandomPoints(width, height) {
    const gen = new RandomPointGenerator(width, height);
    return gen.generateThreePoints();
}