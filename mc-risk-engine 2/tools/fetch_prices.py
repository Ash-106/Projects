#!/usr/bin/env python3
"""Pull adjusted close history into the CSV format the engine expects.

    pip install yfinance
    python3 tools/fetch_prices.py --tickers RELIANCE.NS,HDFCBANK.NS,INFY.NS,TCS.NS \
        --start 2015-01-01 --out data/nifty.csv

Output: date,<ticker1>,<ticker2>,...  one row per trading day, prices only.
Run the backtest on real data before putting any calibration numbers on a CV --
synthetic data is only there so the repo builds and runs out of the box.
"""
import argparse
import sys

def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--tickers", required=True, help="comma-separated")
    p.add_argument("--start", default="2015-01-01")
    p.add_argument("--end", default=None)
    p.add_argument("--out", default="data/prices.csv")
    a = p.parse_args()

    try:
        import yfinance as yf
    except ImportError:
        print("pip install yfinance", file=sys.stderr)
        return 1

    tickers = [t.strip() for t in a.tickers.split(",") if t.strip()]
    df = yf.download(tickers, start=a.start, end=a.end, auto_adjust=True, progress=False)

    close = df["Close"] if "Close" in df else df
    if len(tickers) == 1:
        close = close.to_frame(tickers[0])
    close = close[tickers].dropna(how="any")

    close.to_csv(a.out, index_label="date", float_format="%.6f")
    print(f"wrote {a.out}: {len(close)} rows x {len(tickers)} assets")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
