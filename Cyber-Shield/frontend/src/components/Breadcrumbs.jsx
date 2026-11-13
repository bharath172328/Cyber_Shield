import { ChevronRight } from 'lucide-react';
import { Link, useLocation } from 'react-router-dom';
import { NAV_ITEMS } from './navItems';

export default function Breadcrumbs() {
  const { pathname } = useLocation();
  const current = NAV_ITEMS.find((n) => n.to === pathname);
  return (
    <nav aria-label="Breadcrumb" className="flex items-center gap-1.5 text-sm text-slate-500">
      <Link to="/dashboard" className="hover:text-slate-200">Cyber Shield</Link>
      <ChevronRight className="h-3.5 w-3.5" />
      <span className="text-slate-200">{current?.label || 'Page'}</span>
    </nav>
  );
}
