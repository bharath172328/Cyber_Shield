import { FileBarChart, FileCheck2, FileSearch, KeyRound, LayoutDashboard, Link2, Network, ScrollText, Settings, Users } from 'lucide-react';

export const NAV_ITEMS = [
  { to: '/dashboard', label: 'Dashboard', icon: LayoutDashboard },
  { to: '/password-analyzer', label: 'Password Analyzer', icon: KeyRound },
  { to: '/url-analyzer', label: 'URL Analyzer', icon: Link2 },
  { to: '/network-scanner', label: 'Network Scanner', icon: Network },
  { to: '/file-security', label: 'File Security', icon: FileSearch },
  { to: '/file-integrity', label: 'File Integrity', icon: FileCheck2 },
  { to: '/security-logs', label: 'Security Logs', icon: ScrollText },
  { to: '/reports', label: 'Reports', icon: FileBarChart },
  { to: '/settings', label: 'Settings', icon: Settings },
  { to: '/admin', label: 'Admin', icon: Users, adminOnly: true },
];
