(function() {
    'use strict';
    const global = globalThis;

    /* --------------------------------------------------------------------------
     * 1. Path2D & TextMetrics Specification
     * -------------------------------------------------------------------------- */
    class TextMetrics {
        constructor(text, font) {
            const approxWidthPerChar = (parseFloat(font) || 10) * 0.6;
            this.width = String(text).length * approxWidthPerChar;
            this.actualBoundingBoxLeft = 0;
            this.actualBoundingBoxRight = this.width;
            this.actualBoundingBoxAscent = (parseFloat(font) || 10) * 0.8;
            this.actualBoundingBoxDescent = (parseFloat(font) || 10) * 0.2;
            this.fontBoundingBoxAscent = this.actualBoundingBoxAscent;
            this.fontBoundingBoxDescent = this.actualBoundingBoxDescent;
            this.emHeightAscent = this.actualBoundingBoxAscent;
            this.emHeightDescent = this.actualBoundingBoxDescent;
            this.alphabeticBaseline = 0;
        }
    }

    class Path2D {
        constructor(path) {
            this.ops = [];
            if (path instanceof Path2D) {
                this.ops = [...path.ops];
            }
        }

        beginPath() { this.ops = []; }
        closePath() { this.ops.push({ type: 'closePath' }); }
        moveTo(x, y) { this.ops.push({ type: 'moveTo', x: Number(x), y: Number(y) }); }
        lineTo(x, y) { this.ops.push({ type: 'lineTo', x: Number(x), y: Number(y) }); }
        quadraticCurveTo(cpx, cpy, x, y) { this.ops.push({ type: 'quadraticCurveTo', cpx, cpy, x, y }); }
        bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y) { this.ops.push({ type: 'bezierCurveTo', cp1x, cp1y, cp2x, cp2y, x, y }); }
        arc(x, y, radius, startAngle, endAngle, anticlockwise = false) {
            this.ops.push({ type: 'arc', x, y, radius, startAngle, endAngle, anticlockwise });
        }
        arcTo(x1, y1, x2, y2, radius) {
            this.ops.push({ type: 'arcTo', x1, y1, x2, y2, radius });
        }
        rect(x, y, w, h) {
            this.moveTo(x, y);
            this.lineTo(x + w, y);
            this.lineTo(x + w, y + h);
            this.lineTo(x, y + h);
            this.closePath();
        }
        ellipse(x, y, radiusX, radiusY, rotation, startAngle, endAngle, anticlockwise = false) {
            this.ops.push({ type: 'ellipse', x, y, radiusX, radiusY, rotation, startAngle, endAngle, anticlockwise });
        }
        addPath(path) {
            if (path instanceof Path2D) {
                this.ops.push(...path.ops);
            }
        }
    }

    global.Path2D = global.Path2D || Path2D;
    global.TextMetrics = global.TextMetrics || TextMetrics;

    /* --------------------------------------------------------------------------
     * 2. CanvasGradient Specification
     * -------------------------------------------------------------------------- */
    class CanvasGradient {
        constructor(type, coords) {
            this._type = type; // 'linear' | 'radial'
            this._coords = coords;
            this._stops = [];
        }

        addColorStop(offset, color) {
            offset = Number(offset);
            if (isNaN(offset) || offset < 0 || offset > 1) {
                throw new DOMException(
                    "Failed to execute 'addColorStop' on 'CanvasGradient': The provided offset is outside the range [0.0, 1.0].",
                    "IndexSizeError"
                );
            }
            if (color === undefined || color === null) {
                throw new DOMException(
                    "Failed to execute 'addColorStop' on 'CanvasGradient': The provided color is invalid.",
                    "SyntaxError"
                );
            }

            color = String(color).trim();
            if (!color) {
                throw new DOMException(
                    "Failed to execute 'addColorStop' on 'CanvasGradient': The provided color could not be parsed.",
                    "SyntaxError"
                );
            }

            this._stops.push({ offset, color });
            this._stops.sort((a, b) => a.offset - b.offset);
        }
    }

    /* --------------------------------------------------------------------------
     * 3. CanvasPattern Specification
     * -------------------------------------------------------------------------- */
    class CanvasPattern {
        constructor(image, repetition = 'repeat') {
            this._image = image;
            this._repetition = String(repetition || 'repeat').trim();
            this._transform = null;

            const validReps = new Set(['repeat', 'repeat-x', 'repeat-y', 'no-repeat', '']);
            if (!validReps.has(this._repetition)) {
                throw new DOMException(
                    `Failed to execute 'createPattern' on 'CanvasRenderingContext2D': The provided type '${repetition}' is not a valid repetition mode.`,
                    "SyntaxError"
                );
            }
            if (this._repetition === '') {
                this._repetition = 'repeat';
            }
        }

        setTransform(transform) {
            if (transform && typeof transform === 'object') {
                this._transform = transform;
            } else {
                this._transform = null;
            }
        }
    }

    global.CanvasGradient = global.CanvasGradient || CanvasGradient;
    global.CanvasPattern = global.CanvasPattern || CanvasPattern;

    /* --------------------------------------------------------------------------
     * 4. Patch CanvasRenderingContext2D Prototype
     * -------------------------------------------------------------------------- */
    const ctxProto = (global.CanvasRenderingContext2D && global.CanvasRenderingContext2D.prototype)
        ? global.CanvasRenderingContext2D.prototype
        : null;

    if (ctxProto) {

        ctxProto.measureText = function(text) {
            return new TextMetrics(text, this.font);
        };

        // Factory methods
        ctxProto.createLinearGradient = function(x0, y0, x1, y1) {
            x0 = Number(x0) || 0;
            y0 = Number(y0) || 0;
            x1 = Number(x1) || 0;
            y1 = Number(y1) || 0;
            return new CanvasGradient('linear', { x0, y0, x1, y1 });
        };

        ctxProto.createRadialGradient = function(x0, y0, r0, x1, y1, r1) {
            x0 = Number(x0) || 0;
            y0 = Number(y0) || 0;
            r0 = Number(r0) || 0;
            x1 = Number(x1) || 0;
            y1 = Number(y1) || 0;
            r1 = Number(r1) || 0;

            if (r0 < 0 || r1 < 0) {
                throw new DOMException(
                    "Failed to execute 'createRadialGradient' on 'CanvasRenderingContext2D': The radius provided is negative.",
                    "IndexSizeError"
                );
            }

            return new CanvasGradient('radial', { x0, y0, r0, x1, y1, r1 });
        };

        ctxProto.createPattern = function(image, repetition) {
            if (!image) {
                throw new TypeError(
                    "Failed to execute 'createPattern' on 'CanvasRenderingContext2D': The provided image argument is null or undefined."
                );
            }
            return new CanvasPattern(image, repetition);
        };

        // Ensure fillStyle / strokeStyle setters accept CanvasGradient & CanvasPattern instances
        const descFill = Object.getOwnPropertyDescriptor(ctxProto, 'fillStyle');
        let _customFill = new WeakMap();
        Object.defineProperty(ctxProto, 'fillStyle', {
            get() {
                if (_customFill.has(this)) return _customFill.get(this);
                return descFill && descFill.get ? descFill.get.call(this) : '#000000';
            },
            set(val) {
                if (val instanceof CanvasGradient || val instanceof CanvasPattern) {
                    _customFill.set(this, val);
                } else {
                    _customFill.delete(this);
                    if (descFill && descFill.set) {
                        descFill.set.call(this, String(val));
                    }
                }
            },
            configurable: true,
            enumerable: true
        });

        const descStroke = Object.getOwnPropertyDescriptor(ctxProto, 'strokeStyle');
        let _customStroke = new WeakMap();
        Object.defineProperty(ctxProto, 'strokeStyle', {
            get() {
                if (_customStroke.has(this)) return _customStroke.get(this);
                return descStroke && descStroke.get ? descStroke.get.call(this) : '#000000';
            },
            set(val) {
                if (val instanceof CanvasGradient || val instanceof CanvasPattern) {
                    _customStroke.set(this, val);
                } else {
                    _customStroke.delete(this);
                    if (descStroke && descStroke.set) {
                        descStroke.set.call(this, String(val));
                    }
                }
            },
            configurable: true,
            enumerable: true
        });
    }
})();
