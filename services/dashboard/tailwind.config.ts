import type { Config } from "tailwindcss";
import tailwindcssAnimate from "tailwindcss-animate";

export default {
  darkMode: ["class"],
  content: ["./index.html", "./src/**/*.{ts,tsx}"],
  theme: {
    extend: {
      fontFamily: {
        sans: [
          "Metropolis",
          "-apple-system",
          "BlinkMacSystemFont",
          "Segoe UI",
          "Roboto",
          "Helvetica Neue",
          "sans-serif",
        ],
        display: [
          "Metropolis",
          "-apple-system",
          "BlinkMacSystemFont",
          "SF Pro Display",
          "system-ui",
          "Segoe UI",
          "sans-serif",
        ],
        mono: [
          "ui-monospace",
          "SF Mono",
          "SFMono-Regular",
          "Menlo",
          "Consolas",
          "monospace",
        ],
      },
      fontSize: {
        display: ["2.75rem", { lineHeight: "1", letterSpacing: "-0.025em" }],
        h1: ["1.375rem", { lineHeight: "1.2", letterSpacing: "-0.015em" }],
        h2: ["1rem", { lineHeight: "1.3", letterSpacing: "-0.01em" }],
        body: ["0.9375rem", { lineHeight: "1.4" }],
        small: ["0.8125rem", { lineHeight: "1.4" }],
        tiny: ["0.6875rem", { lineHeight: "1.4" }],
      },
      backgroundImage: {
        "grad-ai": "var(--grad-ai)",
      },
      backdropBlur: {
        glass: "18px",
      },
      boxShadow: {
        card: "var(--sh-card)",
        pop: "var(--sh-pop)",
        glass: "var(--sh-glass)",
      },
      colors: {
        background: "hsl(var(--background))",
        foreground: "hsl(var(--foreground))",
        border: {
          DEFAULT: "hsl(var(--border))",
          strong: "hsl(var(--border-strong))",
        },
        input: "hsl(var(--input))",
        ring: "hsl(var(--ring))",
        primary: {
          DEFAULT: "hsl(var(--primary))",
          foreground: "hsl(var(--primary-foreground))",
          hover: "hsl(var(--primary-hover))",
        },
        secondary: {
          DEFAULT: "hsl(var(--secondary))",
          foreground: "hsl(var(--secondary-foreground))",
        },
        destructive: {
          DEFAULT: "hsl(var(--destructive))",
          foreground: "hsl(var(--destructive-foreground))",
          bg: "hsl(var(--destructive-bg))",
          border: "hsl(var(--destructive-border))",
        },
        muted: {
          DEFAULT: "hsl(var(--muted))",
          foreground: "hsl(var(--muted-foreground))",
        },
        accent: {
          DEFAULT: "hsl(var(--accent))",
          foreground: "hsl(var(--accent-foreground))",
        },
        popover: {
          DEFAULT: "hsl(var(--popover))",
          foreground: "hsl(var(--popover-foreground))",
        },
        card: {
          DEFAULT: "hsl(var(--card))",
          foreground: "hsl(var(--card-foreground))",
        },
        lime: {
          DEFAULT: "hsl(var(--lime))",
          deep: "hsl(var(--lime-deep))",
          foreground: "hsl(var(--lime-foreground))",
        },
        ok: {
          DEFAULT: "hsl(var(--ok))",
          bg: "hsl(var(--ok-bg))",
          fg: "hsl(var(--ok-fg))",
        },
        warning: {
          DEFAULT: "hsl(var(--warning))",
          bg: "hsl(var(--warning-bg))",
          fg: "hsl(var(--warning-fg))",
        },
        danger: {
          DEFAULT: "hsl(var(--danger))",
          bg: "hsl(var(--danger-bg))",
          fg: "hsl(var(--danger-fg))",
        },
        info: {
          DEFAULT: "hsl(var(--info))",
          bg: "hsl(var(--info-bg))",
          fg: "hsl(var(--info-fg))",
        },
        soil: { DEFAULT: "hsl(var(--k-soil))", bg: "hsl(var(--k-soil-bg))" },
        temp: { DEFAULT: "hsl(var(--k-temp))", bg: "hsl(var(--k-temp-bg))" },
        hum: { DEFAULT: "hsl(var(--k-hum))", bg: "hsl(var(--k-hum-bg))" },
        vbat: { DEFAULT: "hsl(var(--k-vbat))", bg: "hsl(var(--k-vbat-bg))" },
      },
      borderRadius: {
        xl: "calc(var(--radius) + 6px)",
        lg: "var(--radius)",
        md: "calc(var(--radius) - 2px)",
        sm: "calc(var(--radius) - 4px)",
        pill: "999px",
      },
      keyframes: {
        "gh-pulse": {
          "0%": { boxShadow: "0 0 0 0 hsl(var(--primary) / 0.4)" },
          "70%": { boxShadow: "0 0 0 7px hsl(var(--primary) / 0)" },
          "100%": { boxShadow: "0 0 0 0 hsl(var(--primary) / 0)" },
        },
        "gh-blink": { "50%": { opacity: "0.25" } },
        "gh-fall": {
          "0%": { transform: "translateY(-10px) scaleY(0.6)", opacity: "0" },
          "10%": { opacity: "0.65" },
          "90%": { opacity: "0.65" },
          "100%": { transform: "translateY(120px) scaleY(1.1)", opacity: "0" },
        },
      },
      animation: {
        "gh-pulse": "gh-pulse 2s ease-out infinite",
        "gh-blink": "gh-blink 1.4s ease-in-out infinite",
        "gh-fall": "gh-fall 1.1s linear infinite",
      },
    },
  },
  plugins: [tailwindcssAnimate],
} satisfies Config;
