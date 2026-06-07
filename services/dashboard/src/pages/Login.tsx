import { useForm } from "react-hook-form";
import { zodResolver } from "@hookform/resolvers/zod";
import { useNavigate, useLocation } from "react-router-dom";
import { toast } from "sonner";
import { AlertCircle, Leaf } from "lucide-react";

import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { Card, CardContent } from "@/components/ui/card";
import {
  Form,
  FormControl,
  FormField,
  FormItem,
  FormLabel,
  FormMessage,
} from "@/components/ui/form";

import { useAuthStore } from "@/lib/auth";
import { useLoginMutation } from "@/hooks/useLoginMutation";
import { type LoginFormValues, loginFormSchema } from "@/schemas/auth";

const BACKDROP =
  "radial-gradient(120% 90% at 15% 10%, color-mix(in srgb, hsl(var(--lime)) 14%, transparent), transparent 55%)," +
  "radial-gradient(100% 80% at 90% 90%, color-mix(in srgb, hsl(var(--primary)) 12%, transparent), transparent 60%)," +
  "hsl(var(--background))";

export default function Login() {
  const navigate = useNavigate();
  const location = useLocation();
  const login = useAuthStore((s) => s.login);
  const mutation = useLoginMutation();

  const form = useForm<LoginFormValues>({
    resolver: zodResolver(loginFormSchema),
    defaultValues: { username: "", password: "" },
  });

  const from =
    (location.state as { from?: { pathname: string } } | null)?.from?.pathname ??
    "/";

  function onSubmit(values: LoginFormValues) {
    mutation.mutate(values, {
      onSuccess: (data) => {
        login(data.admin_token, values.username);
        toast.success(`Signed in as ${values.username}`);
        navigate(from, { replace: true });
      },
      onError: (err: unknown) => {
        const msg =
          err && typeof err === "object" && "detail" in err
            ? String((err as { detail: string }).detail)
            : "Login failed";
        toast.error(msg);
      },
    });
  }

  return (
    <div
      className="flex min-h-screen items-center justify-center overflow-hidden p-6"
      style={{ background: BACKDROP }}
    >
      <div className="flex w-full max-w-[400px] flex-col gap-6">
        {/* brand block */}
        <div className="flex flex-col items-center gap-3.5 text-center">
          <span
            className="grid h-[52px] w-[52px] place-items-center rounded-[15px] shadow-lg"
            style={{
              background:
                "linear-gradient(140deg, hsl(var(--primary)), hsl(var(--lime)))",
              color: "hsl(var(--lime-foreground))",
            }}
          >
            <Leaf className="h-7 w-7" strokeWidth={1.9} />
          </span>
          <div>
            <h1 className="text-[26px] font-semibold tracking-tight text-foreground">
              Greenhouse Hub
            </h1>
            <p className="mt-1 text-sm text-muted-foreground">
              Sign in to manage your greenhouse
            </p>
          </div>
        </div>

        <Card className="border-border shadow-lg">
          <CardContent className="p-6">
            <Form {...form}>
              <form
                onSubmit={form.handleSubmit(onSubmit)}
                className="flex flex-col gap-4"
                aria-label="login"
              >
                {mutation.isError && (
                  <div
                    role="alert"
                    className="flex items-start gap-2.5 rounded-lg border p-3 text-sm"
                    style={{
                      background: "hsl(var(--destructive-bg))",
                      borderColor: "hsl(var(--destructive-border))",
                      color: "hsl(var(--danger-fg))",
                    }}
                  >
                    <AlertCircle className="mt-0.5 h-4 w-4 shrink-0" />
                    <div>
                      <span className="font-semibold">
                        Incorrect username or password.
                      </span>{" "}
                      Check your credentials and try again.
                    </div>
                  </div>
                )}

                <FormField
                  control={form.control}
                  name="username"
                  render={({ field }) => (
                    <FormItem>
                      <FormLabel>Username</FormLabel>
                      <FormControl>
                        <Input autoComplete="username" {...field} />
                      </FormControl>
                      <FormMessage />
                    </FormItem>
                  )}
                />
                <FormField
                  control={form.control}
                  name="password"
                  render={({ field }) => (
                    <FormItem>
                      <FormLabel>Password</FormLabel>
                      <FormControl>
                        <Input
                          type="password"
                          autoComplete="current-password"
                          placeholder="••••••••"
                          {...field}
                        />
                      </FormControl>
                      <FormMessage />
                    </FormItem>
                  )}
                />
                <Button
                  type="submit"
                  className="mt-1 h-11 w-full text-[15px]"
                  disabled={mutation.isPending}
                >
                  {mutation.isPending ? "Signing in…" : "Sign in"}
                </Button>
              </form>
            </Form>

            <p className="mt-4 text-center text-xs leading-relaxed text-muted-foreground">
              Single-owner hub · no public sign-up. First admin token is created
              via CLI bootstrap.
            </p>
          </CardContent>
        </Card>
      </div>
    </div>
  );
}
