import { ShieldQuestion } from 'lucide-react';
import { Link } from 'react-router-dom';
import Button from '../components/Button';
import EmptyState from '../components/EmptyState';

export default function NotFound() {
  return (
    <div className="flex min-h-screen items-center justify-center p-6">
      <EmptyState icon={ShieldQuestion} title="404 · Page not found" description="The page you are looking for does not exist or has moved."
        action={<Link to="/dashboard"><Button>Back to dashboard</Button></Link>} />
    </div>
  );
}
