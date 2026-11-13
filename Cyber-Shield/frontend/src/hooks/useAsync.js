import { useCallback, useEffect, useRef, useState } from 'react';
import { errorMessage } from '../services/api';

/** Wraps an async function with loading / error / data state. */
export function useAsync(fn, { immediate = false, deps = [] } = {}) {
  const [state, setState] = useState({ data: null, loading: immediate, error: null });
  const alive = useRef(true);
  useEffect(() => { alive.current = true; return () => { alive.current = false; }; }, []);

  const run = useCallback(async (...args) => {
    setState((s) => ({ ...s, loading: true, error: null }));
    try {
      const data = await fn(...args);
      if (alive.current) setState({ data, loading: false, error: null });
      return data;
    } catch (err) {
      if (alive.current) setState((s) => ({ ...s, loading: false, error: errorMessage(err) }));
      throw err;
    }
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, deps);

  useEffect(() => { if (immediate) run().catch(() => {}); }, [run, immediate]);
  return { ...state, run, setData: (data) => setState((s) => ({ ...s, data })) };
}
