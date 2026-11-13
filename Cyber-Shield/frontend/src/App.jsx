import { Navigate, Route, Routes } from 'react-router-dom';
import ProtectedRoute from './components/ProtectedRoute';
import AppLayout from './layouts/AppLayout';
import AuthLayout from './layouts/AuthLayout';
import Admin from './pages/Admin';
import Dashboard from './pages/Dashboard';
import FileIntegrity from './pages/FileIntegrity';
import FileSecurity from './pages/FileSecurity';
import Login from './pages/Login';
import NetworkScanner from './pages/NetworkScanner';
import NotFound from './pages/NotFound';
import PasswordAnalyzer from './pages/PasswordAnalyzer';
import Register from './pages/Register';
import Reports from './pages/Reports';
import SecurityLogs from './pages/SecurityLogs';
import Settings from './pages/Settings';
import UrlAnalyzer from './pages/UrlAnalyzer';

export default function App() {
  return (
    <Routes>
      <Route element={<AuthLayout />}>
        <Route path="/login" element={<Login />} />
        <Route path="/register" element={<Register />} />
      </Route>

      <Route element={<ProtectedRoute><AppLayout /></ProtectedRoute>}>
        <Route path="/dashboard" element={<Dashboard />} />
        <Route path="/password-analyzer" element={<PasswordAnalyzer />} />
        <Route path="/url-analyzer" element={<UrlAnalyzer />} />
        <Route path="/network-scanner" element={<NetworkScanner />} />
        <Route path="/file-security" element={<FileSecurity />} />
        <Route path="/file-integrity" element={<FileIntegrity />} />
        <Route path="/security-logs" element={<SecurityLogs />} />
        <Route path="/reports" element={<Reports />} />
        <Route path="/settings" element={<Settings />} />
        <Route path="/admin" element={<ProtectedRoute adminOnly><Admin /></ProtectedRoute>} />
      </Route>

      <Route path="/" element={<Navigate to="/dashboard" replace />} />
      <Route path="*" element={<NotFound />} />
    </Routes>
  );
}
