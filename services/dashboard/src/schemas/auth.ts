import { z } from "zod";

export const loginFormSchema = z.object({
  username: z.string().min(1, "Username is required"),
  password: z.string().min(8, "Password must be at least 8 characters"),
});

export type LoginFormValues = z.infer<typeof loginFormSchema>;

export const loginResponseSchema = z.object({
  admin_token: z.string().length(64),
  name: z.string(),
});

export type LoginResponse = z.infer<typeof loginResponseSchema>;
