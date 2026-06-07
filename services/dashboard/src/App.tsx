import { Toaster } from "@/components/ui/sonner";
import { AppRoutes } from "./routes";

export default function App() {
  return (
    <>
      <AppRoutes />
      <Toaster richColors position="top-right" />
    </>
  );
}
